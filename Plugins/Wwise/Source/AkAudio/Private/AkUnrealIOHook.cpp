/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2023 Audiokinetic Inc.
*******************************************************************************/

#include "AkUnrealIOHook.h"

#include "AkAudioDevice.h"
#include "AkMediaAsset.h"
#include "AkUnrealHelper.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/AsyncFileHandle.h"
#include "HAL/FileManager.h"
#if UE_5_0_OR_LATER
#include "HAL/PlatformFileManager.h"
#else
#include "HAL/PlatformFilemanager.h"
#endif
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "Async/Async.h"
#endif

FCriticalSection FAkUnrealIOHook::MediaMapCriticalSection;
TMap<uint32, TArray<FAkBulkData>> FAkUnrealIOHook::StreamingMediaBulkDataMap;
TMap<FString, TArray<FAkBulkData>> FAkUnrealIOHook::ExternalMediaBulkDataMap;

struct AkFileCustomParam
{
	virtual ~AkFileCustomParam() {}
	virtual AKRESULT DoWork(AkAsyncIOTransferInfo& io_transferInfo) = 0;

	static void SetupAkFileDesc(AkFileDesc& fileDesc, AkFileCustomParam* customParam)
	{
		fileDesc.pCustomParam = customParam;
		fileDesc.uCustomParamSize = sizeof(AkFileCustomParam);
	}

	static AkFileCustomParam* GetFileCustomParam(const AkFileDesc& fileDesc)
	{
		if (fileDesc.uCustomParamSize == sizeof(AkFileCustomParam))
		{
			return reinterpret_cast<AkFileCustomParam*>(fileDesc.pCustomParam);
		}

		UE_LOG(LogAkAudio, Log, TEXT("AkFileCustomParam::GetFileCustomParam: Could not get CustomParam from fileDesc."));
		return nullptr;
	}
};

struct AkReadAssetCustomParam : public AkFileCustomParam
{
public:
	AkReadAssetCustomParam(FAkBulkData MediaAssetBulkData) : MediaStreamedChunkData(MediaAssetBulkData)
	{
	}

	virtual ~AkReadAssetCustomParam()
	{
		for (auto& ioRequest : IORequests)
		{
			if (ioRequest)
			{
				ioRequest->WaitCompletion();
				delete ioRequest;
			}
		}
	}

	AKRESULT DoWork(AkAsyncIOTransferInfo& io_transferInfo) override
	{
		auto AkTransferInfo = io_transferInfo;

		for (auto& ioRequest : IORequests)
		{
			if (ioRequest && ioRequest->PollCompletion())
			{
				ioRequest->WaitCompletion();
				delete ioRequest;
				ioRequest = nullptr;
			}
		}

		IORequests.Remove(nullptr);

		BulkDataRequestCompletedCallback AsyncFileCallBack = [AkTransferInfo](bool bWasCancelled, ReadRequestArgumentType* ReadRequest) mutable
		{
			if (AkTransferInfo.pCallback)
			{
				AkTransferInfo.pCallback(&AkTransferInfo, AK_Success);
			}
		};

		if (!MediaStreamedChunkData.IsValid())
		{
			UE_LOG(LogAkAudio, Warning, TEXT("AkReadAssetCustomParam::DoWork: Media Bulk data is invalid, closing stream."));
			return AK_Fail;
		}

		BulkDataIORequest* PendingReadRequest = MediaStreamedChunkData->CreateStreamingRequest(AkTransferInfo.uFilePosition, AkTransferInfo.uRequestedSize, AIOP_High, &AsyncFileCallBack, (uint8*)AkTransferInfo.pBuffer);
		if (PendingReadRequest)
		{
			IORequests.Add(PendingReadRequest);
		}
		return AK_Success;
	}

public:
	TSharedPtr<FByteBulkData,  ESPMode::ThreadSafe> MediaStreamedChunkData;
	TArray<BulkDataIORequest*, TInlineAllocator<8>> IORequests;
};

struct AkEditorReadCustomParam : AkFileCustomParam
{
public:
#if UE_4_25_OR_LATER
#if AK_RUNTIME_BULKDATA
	AkEditorReadCustomParam(FByteBulkData& BulkData)
#else
	AkEditorReadCustomParam(const FByteBulkData& BulkData)
#endif
#else
	AkEditorReadCustomParam(const FByteBulkData& BulkData)
#endif
	{
		RawData = BulkData.LockReadOnly();
		BulkData.Unlock();
	}

	~AkEditorReadCustomParam()
	{
		RawData = nullptr;
	}

	AKRESULT DoWork(AkAsyncIOTransferInfo& io_transferInfo) override
	{
		if (io_transferInfo.pCallback)
		{
			FMemory::Memcpy(io_transferInfo.pBuffer, reinterpret_cast<const uint8*>(RawData) + io_transferInfo.uFilePosition, io_transferInfo.uRequestedSize);
			io_transferInfo.pCallback(&io_transferInfo, AK_Success);
		}

		return AK_Success;
	}

public:
	const void* RawData = nullptr;
};

struct AkWriteFileCustomParam : AkFileCustomParam
{
private:
	IFileHandle* FileHandle;

public:
	AkWriteFileCustomParam(IFileHandle* handle)
	: FileHandle(handle)
	{}

	virtual ~AkWriteFileCustomParam()
	{
		if (FileHandle)
		{
			delete FileHandle;
		}
	}

	virtual AKRESULT DoWork(AkAsyncIOTransferInfo& io_transferInfo) override
	{
		FileHandle->Seek(io_transferInfo.uFilePosition);

		if (FileHandle->Write((uint8*)io_transferInfo.pBuffer, io_transferInfo.uBufferSize))
		{
			if (io_transferInfo.pCallback)
			{
				io_transferInfo.pCallback(&io_transferInfo, AK_Success);
			}

			return AK_Success;
		}

		UE_LOG(LogAkAudio, Log, TEXT("AkWriteFileCustomParam::DoWork: Failed to write to buffer. Closing stream."));
		return AK_Fail;
	}
};

FAkUnrealIOHook::FAkUnrealIOHook()
{
	assetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	localizedPackagePath = AkUnrealHelper::GetLocalizedAssetPackagePath();
	mediaPackagePath = FPaths::Combine(AkUnrealHelper::GetBaseAssetPackagePath(), AkUnrealHelper::MediaFolderName);
}

FAkUnrealIOHook::~FAkUnrealIOHook()
{
	if (AK::StreamMgr::GetFileLocationResolver() == this)
	{
		AK::StreamMgr::SetFileLocationResolver(nullptr);
	}

	AK::StreamMgr::DestroyDevice(m_deviceID);
}

bool FAkUnrealIOHook::Init(const AkDeviceSettings& in_deviceSettings)
{
	if (in_deviceSettings.uSchedulerTypeFlags != AK_SCHEDULER_DEFERRED_LINED_UP)
	{
		AKASSERT(!"CAkDefaultIOHookDeferred I/O hook only works with AK_SCHEDULER_DEFERRED_LINED_UP devices");
		return false;
	}

	// If the Stream Manager's File Location Resolver was not set yet, set this object as the 
	// File Location Resolver (this I/O hook is also able to resolve file location).
	if (!AK::StreamMgr::GetFileLocationResolver())
	{
		AK::StreamMgr::SetFileLocationResolver(this);
	}

	// Create a device in the Stream Manager, specifying this as the hook.
	m_deviceID = AK::StreamMgr::CreateDevice(in_deviceSettings, this);
	return m_deviceID != AK_INVALID_DEVICE_ID;
}

AKRESULT FAkUnrealIOHook::Open(
	const AkOSChar*			in_pszFileName,
	AkOpenMode				in_eOpenMode,
	AkFileSystemFlags*		in_pFlags,
	bool&					io_bSyncOpen,
	AkFileDesc&			out_fileDesc
)
{
	UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::Open: Requesting to open file name '%s' for streaming."), in_pszFileName);
	CleanFileDescriptor(out_fileDesc);

	if (!io_bSyncOpen)
	{
		// Our open is blocking, wait for the IO thread to call us back
		return AK_Success;
	}

	if (in_eOpenMode == AK_OpenModeRead && in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL)
	{
		auto AssetName = FPaths::GetBaseFilename(in_pszFileName);
		auto ExternalSourceData = GetExternalSourceAssetData(AssetName);
		if (!ExternalSourceData.IsValid())
		{
			UE_LOG(LogAkAudio, Log, TEXT("FAkUnrealIOHook::Open: External source data for file named '%s' is not in ExternalMediaBulkDataMap."), in_pszFileName);
			return AK_Fail;
		}
		auto result = OpenStreamedChunk(ExternalSourceData, out_fileDesc);
		//Free the datachunk that was added for this External Source
		RemoveExternalMedia(AssetName);
		return result;
	}

	if (in_eOpenMode == AK_OpenModeWrite || in_eOpenMode == AK_OpenModeWriteOvrwr)
	{
		const auto TargetDirectory = FPaths::ProjectSavedDir() / TEXT("Wwise");
		static bool TargetDirectoryExists = false;
		if (!TargetDirectoryExists && !FPaths::DirectoryExists(TargetDirectory))
		{
			TargetDirectoryExists = true;
			if (!FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*TargetDirectory))
			{
				UE_LOG(LogAkAudio, Error, TEXT("Cannot create writable directory at %s"), *TargetDirectory);
				return AK_NotImplemented;
			}
		}
		auto fullPath = FPaths::Combine(TargetDirectory, FString(in_pszFileName));
		if (auto FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*fullPath))
		{
			AkFileCustomParam::SetupAkFileDesc(out_fileDesc, new AkWriteFileCustomParam(FileHandle));
			return AK_Success;
		}
		else
		{
			UE_LOG(LogAkAudio, Error, TEXT("Cannot open file %s for write."), *fullPath);
		}
	}

	return AK_NotImplemented;
}

AKRESULT FAkUnrealIOHook::Open(
	AkFileID				in_fileID,          // File ID.
	AkOpenMode				in_eOpenMode,       // Open mode.
	AkFileSystemFlags*		in_pFlags,			// Special flags. Can pass NULL.
	bool&					io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
	AkFileDesc&			out_fileDesc        // Returned file descriptor.
)
{
	UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::Open: Requesting to open file with ID '%d' for streaming."), in_fileID);

	CleanFileDescriptor(out_fileDesc);
	if (!io_bSyncOpen)
	{
		// Our open is blocking, wait for the IO thread to call us back
		return AK_Success;
	}

	if (in_eOpenMode != AK_OpenModeRead)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("FAkUnrealIOHook::Open: Opening in mode other than read is not supported."));
		return AK_NotImplemented;
	}

	return OpenStreamedChunk(GetStreamingAssetData((uint32)in_fileID), out_fileDesc);
}

FAkBulkData FAkUnrealIOHook::GetStreamingAssetData(const uint32 AssetID)
{
	FScopeLock Lock(&MediaMapCriticalSection);
	if (!StreamingMediaBulkDataMap.Contains(AssetID))
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkUnrealIOHook::GetStreamingAssetData: Could not find streaming file ID %d in the Media Map. Is the UAkMediaAsset properly loaded?"), AssetID);
		return {};
	}
	auto Media = StreamingMediaBulkDataMap.FindRef(AssetID);
	if (Media.Num() == 0) 
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkUnrealIOHook::GetStreamingAssetData: No media associated with file ID %d currently loaded."), AssetID);
		return {};
	}

	for (auto Entry : Media)
	{
		if (Entry.IsValid())
		{
			return Entry;
		}
	}

	UE_LOG(LogAkAudio, Warning, TEXT("FAkUnrealIOHook::GetStreamingAsset: Could not valid streaming asset with ID %d in the Media Map."), AssetID);
	return {};
}

FAkBulkData FAkUnrealIOHook::GetExternalSourceAssetData(const FString& AssetName)
{	
	UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::GetExternalSourceAsset: Getting external source asset '%s'."), *AssetName);
	FScopeLock Lock(&MediaMapCriticalSection);
	if (!ExternalMediaBulkDataMap.Contains(AssetName))
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkUnrealIOHook::GetExternalSourceAsset: Could not find external source file ID %s in the Media Map. Is the UAkExternalMediaAsset properly loaded?"), *AssetName);
		return {};
	}
	auto Media = ExternalMediaBulkDataMap.FindRef(AssetName);
	if (Media.Num() == 0)
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkUnrealIOHook::GetExternalSourceAsset: No media associated with file name %s currently loaded."), *AssetName);
		return {};
	}

	for (auto Entry : Media)
	{
		if (Entry.IsValid())
		{
			return Entry;
		}
	}

	UE_LOG(LogAkAudio, Warning, TEXT("FAkUnrealIOHook::GetExternalSourceAsset: Could not find valid External source streaming asset with name '%s' in the External Media Map."), *AssetName);
	return {};
}

AKRESULT FAkUnrealIOHook::OpenStreamedChunk(FAkBulkData StreamedChunkData, AkFileDesc& OutFileDesc)
{
	if (!StreamedChunkData.IsValid())
	{
		UE_LOG(LogAkAudio, Warning, TEXT("FAkUnrealIOHook::OpenStreamedChunk: Streamed Chunk is not valid, stream will not be opened."));
		return AK_Fail;
	}
	OutFileDesc.iFileSize = StreamedChunkData->GetBulkDataSize();
#if WITH_EDITOR
#if UE_5_0_OR_LATER
	if (StreamedChunkData->GetBulkDataOffsetInFile() == -1 || !StreamedChunkData->DoesExist())
#else
	if (StreamedChunkData->GetBulkDataOffsetInFile() == -1 || StreamedChunkData->GetFilename().Len() == 0)
#endif
	{
		auto customParam = new AkEditorReadCustomParam(StreamedChunkData.ToSharedRef().Get());
		AkFileCustomParam::SetupAkFileDesc(OutFileDesc, customParam);
		return AK_Success;
	}
#endif

	auto CustomParam = new AkReadAssetCustomParam(StreamedChunkData);
	AkFileCustomParam::SetupAkFileDesc(OutFileDesc, CustomParam);
	return AK_Success;
}

AKRESULT FAkUnrealIOHook::Read(
	AkFileDesc &			in_fileDesc,
	const AkIoHeuristics &	in_heuristics,
	AkAsyncIOTransferInfo & io_transferInfo
)
{
	auto fileCustomParam = AkFileCustomParam::GetFileCustomParam(in_fileDesc);
	if (fileCustomParam)
	{
		return fileCustomParam->DoWork(io_transferInfo);
	}

	UE_LOG(LogAkAudio, Log, TEXT("FAkUnrealIOHook::Read: CustomParam is null. Closing stream."));
	return AK_Fail;
}

AKRESULT FAkUnrealIOHook::Write(
	AkFileDesc &			in_fileDesc,
	const AkIoHeuristics&	in_heuristics,
	AkAsyncIOTransferInfo& io_transferInfo
)
{
	auto fileCustomParam = AkFileCustomParam::GetFileCustomParam(in_fileDesc);
	if (fileCustomParam)
	{
		return fileCustomParam->DoWork(io_transferInfo);
	}
	UE_LOG(LogAkAudio, Log, TEXT("FAkUnrealIOHook::Read: CustomParam is null. Closing stream."));
	return AK_Fail;
}

void FAkUnrealIOHook::Cancel(
	AkFileDesc &			in_fileDesc,
	AkAsyncIOTransferInfo & io_transferInfo,
	bool & io_bCancelAllTransfersForThisFile
)
{
	UE_LOG(LogAkAudio, VeryVerbose, TEXT("FAkUnrealIOHook::Cancel: Doing nothing."));
}

AKRESULT FAkUnrealIOHook::Close(AkFileDesc& in_fileDesc)
{

	UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::Close: Closing stream."));
	auto fileCustomParam = AkFileCustomParam::GetFileCustomParam(in_fileDesc);
	if (fileCustomParam)
	{
		delete fileCustomParam;
	}

	AkFileCustomParam::SetupAkFileDesc(in_fileDesc, nullptr);
	return AK_Success;
}

// Returns the block size for the file or its storage device. 
AkUInt32 FAkUnrealIOHook::GetBlockSize(AkFileDesc& in_fileDesc)
{
	return 1;
}

// Returns a description for the streaming device above this low-level hook.
void FAkUnrealIOHook::GetDeviceDesc(AkDeviceDesc& 
#if !defined(AK_OPTIMIZED)
	out_deviceDesc
#endif
)
{
#if !defined(AK_OPTIMIZED)
	// Deferred scheduler.
	out_deviceDesc.deviceID = m_deviceID;
	out_deviceDesc.bCanRead = true;
	out_deviceDesc.bCanWrite = true;
	AK_CHAR_TO_UTF16(out_deviceDesc.szDeviceName, "UnrealIODevice", AK_MONITOR_DEVICENAME_MAXLENGTH);
	out_deviceDesc.szDeviceName[AK_MONITOR_DEVICENAME_MAXLENGTH - 1] = '\0';
	out_deviceDesc.uStringSize = (AkUInt32)AKPLATFORM::AkUtf16StrLen(out_deviceDesc.szDeviceName) + 1;
#endif
}

// Returns custom profiling data: 1 if file opens are asynchronous, 0 otherwise.
AkUInt32 FAkUnrealIOHook::GetDeviceData()
{
	return 1;
}

void FAkUnrealIOHook::CleanFileDescriptor(AkFileDesc& out_fileDesc)
{
	out_fileDesc.uSector = 0;
	out_fileDesc.deviceID = m_deviceID;

	auto fileCustomParam = AkFileCustomParam::GetFileCustomParam(out_fileDesc);
	if (fileCustomParam)
	{
		delete fileCustomParam;
	}

	AkFileCustomParam::SetupAkFileDesc(out_fileDesc, nullptr);

	out_fileDesc.iFileSize = 0;
}

void FAkUnrealIOHook::AddStreamingMedia(UAkMediaAsset* MediaToAdd)
{
	if (MediaToAdd)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::AddStreamingMedia: Adding streaming media to StreamingMediaBulkDataMap. Name: %s, ID: %d."), *MediaToAdd->MediaName, MediaToAdd->Id);
		if (auto StreamedChunk = MediaToAdd->GetStreamedChunk())
		{
			if (!StreamedChunk->Data.IsValid())
			{
				UE_LOG(LogAkAudio, Error, TEXT("FAkUnrealIOHook::AddStreamingMedia: StreamedChunk Data for media with ID '%d' is invalid."), MediaToAdd->Id);
				return;
			}
			auto MediaBulkData = FAkBulkData(StreamedChunk->Data);
			FScopeLock Lock(&MediaMapCriticalSection);
			if (LIKELY(!StreamingMediaBulkDataMap.Contains(MediaToAdd->Id)))
			{
				StreamingMediaBulkDataMap.Add(MediaToAdd->Id, TArray<FAkBulkData>({ MediaBulkData }));
			}
			else
			{
				UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::AddStreamingMedia: StreamingMediaBulkDataMap already contains media with ID '%d', appending."), MediaToAdd->Id);
				StreamingMediaBulkDataMap.Find(MediaToAdd->Id)->Push(MediaBulkData);
			}
		}
	}
}

void FAkUnrealIOHook::RemoveStreamingMedia(UAkMediaAsset* MediaToRemove)
{
	if (MediaToRemove)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::RemoveStreamingMedia: Removing media from StreamingMediaBulkDataMap. Name: %s, ID: %d"), *MediaToRemove->MediaName, MediaToRemove->Id);

		if (auto StreamedChunk = MediaToRemove->GetStreamedChunk())
		{
			FScopeLock Lock(&MediaMapCriticalSection);
			auto MediaDataArray = StreamingMediaBulkDataMap.FindRef(MediaToRemove->Id);
			MediaDataArray.Remove(StreamedChunk->Data);
			if (LIKELY(MediaDataArray.Num() == 0))
			{
				UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::RemoveStreamingMedia: Removing StreamingMediaBulkDataMap entry with ID '%d'."), MediaToRemove->Id);
				StreamingMediaBulkDataMap.Remove(MediaToRemove->Id);
			}
		}
		
	}
}

//An FAkSDKExternalSourceArray is supposed to be created each time a PostEvent call with external sources is made.
//This will call AddExternalMedia for each streamed external source.
//The ExternalMediaBulkDataMap therefore serves as a queue of DataChunks that are meant to opened once. 
//The CustomParam should keep the DataChunk "alive" as long as it is needed for the stream.
void FAkUnrealIOHook::AddExternalMedia(UAkExternalMediaAsset* MediaToAdd)
{
	if (MediaToAdd)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::AddExternalMedia: Adding streaming media to ExternalMediaBulkDataMap. Name: %s, ID: %d"), *MediaToAdd->MediaName, MediaToAdd->Id);
		MediaToAdd->NumStreamingHandles.Increment();
		if (auto StreamedChunk = MediaToAdd->GetStreamedChunk())
		{
			if (!StreamedChunk->Data.IsValid())
			{
				UE_LOG(LogAkAudio, Error, TEXT("FAkUnrealIOHook::AddStreamingMedia: StreamedChunk Data for external media with ID '%d' is invalid."), MediaToAdd->Id);
				return;
			}
			auto MediaBulkData = FAkBulkData(StreamedChunk->Data);
			FScopeLock Lock(&MediaMapCriticalSection);
			if (!ExternalMediaBulkDataMap.Contains(MediaToAdd->GetName()))
			{
				ExternalMediaBulkDataMap.Add(MediaToAdd->GetName(), TArray<FAkBulkData>({ MediaBulkData }));
			}
			else
			{
				UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::AddStreamingMedia: ExternalMediaBulkDataMap already contains media with ID '%d', appending."), MediaToAdd->Id);
				//Adds to end of array
				ExternalMediaBulkDataMap.Find(MediaToAdd->GetName())->Push(MediaBulkData);
			}
		}
	}
	else
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkUnrealIOHook::AddExternalMedia: Pointer to External media is invalid."));
	}
}

void FAkUnrealIOHook::RemoveExternalMedia(const FString& ExternalMediaName)
{
	UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::RemoveExternalMedia: Removing external streaming media from ExternalMediaBulkDataMap. Name: %s"), *ExternalMediaName);
	FScopeLock Lock(&MediaMapCriticalSection);
	if (auto MediaDataArray = ExternalMediaBulkDataMap.Find(ExternalMediaName))
	{
		//Remove the first chunk in the array
		MediaDataArray->Pop();
		if (LIKELY(MediaDataArray->Num() == 0))
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("FAkUnrealIOHook::RemoveExternalMedia: Removing ExternalMediaBulkDataMap entry with Name '%s'."), *ExternalMediaName);
			ExternalMediaBulkDataMap.Remove(ExternalMediaName);
		}
	}
}