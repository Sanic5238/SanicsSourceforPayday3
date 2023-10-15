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

#include "AkMediaAsset.h"

#include "AkAudioDevice.h"
#include "AkUnrealHelper.h"
#include "AkUnrealIOHook.h"
#include "AkMediaMemoryManager.h"
#include "HAL/PlatformProperties.h"
#include "Core/Public/Modules/ModuleManager.h"
#include "Misc/ScopeExit.h"
#include "Platforms/AkPlatformInfo.h"

#if WITH_EDITOR
#include "Interfaces/ITargetPlatform.h"
#else
#include "Async/Async.h"
#endif

FAkMediaDataChunk::FAkMediaDataChunk() { }

#if WITH_EDITOR
FAkMediaDataChunk::FAkMediaDataChunk(IFileHandle* FileHandle, int64 BytesToRead, uint32 BulkDataFlags, FCriticalSection* DataWriteLock, bool IsPrefetch)
	: IsPrefetch(IsPrefetch)
{
	FScopeLock DataLock(DataWriteLock);
	Data = TSharedPtr<FByteBulkData, ESPMode::ThreadSafe>(new FByteBulkData());
	Data->SetBulkDataFlags(BulkDataFlags);
	Data->Lock(EBulkDataLockFlags::LOCK_READ_WRITE);
	FileHandle->Read(reinterpret_cast<uint8*>(Data->Realloc(BytesToRead)), BytesToRead);
	Data->Unlock();
}
#endif

void FAkMediaDataChunk::Serialize(FArchive& Ar, UObject* Owner)
{
	Ar << IsPrefetch;
	if (Ar.IsLoading())
	{
		Data = TSharedPtr<FByteBulkData, ESPMode::ThreadSafe>(new FByteBulkData());
		Data->Serialize(Ar, Owner);
	}
	if (Ar.IsSaving())
	{
		if (!Data.IsValid())
		{
			UE_LOG(LogAkAudio, Warning, TEXT("FAkMediaDataChunk::Serialize: Chunk data is invalid.Creating a new empty chunk to serialize."))
			Data = TSharedPtr<FByteBulkData, ESPMode::ThreadSafe>(new FByteBulkData());
		}
		Data->Serialize(Ar, Owner);
	}
}

uint32 UAkMediaAssetData::GetParentMediaId() const
{
	UAkMediaAsset* ParentMediaAsset = Cast<UAkMediaAsset>(GetOuter());
	return ParentMediaAsset ? ParentMediaAsset->Id : 0;
}

FString UAkMediaAssetData::GetParentMediaName() const
{
	UAkMediaAsset* ParentMediaAsset = Cast<UAkMediaAsset>(GetOuter());
	return ParentMediaAsset ? ParentMediaAsset->MediaName : "Unknown";
}

UAkMediaAssetData::LoadState UAkMediaAssetData::GetLoadState() const
{
	return State;
}

auto UAkMediaAssetData::GetPreloadDependencies(TArray<UObject*>& OutDeps) -> void
{
	Super::GetPreloadDependencies(OutDeps);
	OutDeps.Add(GetOuter());
}

bool UAkMediaAssetData::IsCurrentWwisePlatform(const FString& AssetDataPlatform)
{
	static UAkPlatformInfo* CurrentPlatformInfo = nullptr;
	if (!CurrentPlatformInfo)
	{
		CurrentPlatformInfo = UAkPlatformInfo::GetAkPlatformInfo(FPlatformProperties::IniPlatformName());
	}

	if(LIKELY(CurrentPlatformInfo))
	{
		return CurrentPlatformInfo->IsCurrentWwisePlatform(AssetDataPlatform);
	}

	return AssetDataPlatform == FPlatformProperties::IniPlatformName();
}


void UAkMediaAssetData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::Serialize: Media is (%s - %u)"), *GetParentMediaName(), GetParentMediaId());

	int32 numChunks = DataChunks.Num();
	Ar << numChunks;

	if (Ar.IsLoading())
	{
		DataChunks.Empty();
		for (int32 i = 0; i < numChunks; ++i)
		{
			DataChunks.Add(new FAkMediaDataChunk());
		}
	}

	for (int32 i = 0; i < numChunks; ++i)
	{
		DataChunks[i].Serialize(Ar, this);
	}

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

#if WITH_EDITOR
	if (AssetPlatform.IsEmpty())
	{
		UE_LOG(LogAkAudio, Warning, TEXT("UAkMediaAssetData::Serialize: Media data %s in %s is not up to date. Please regenerate sound data."), *GetName(), *GetPackage()->GetFName().ToString());
	}
#endif

	if (Ar.IsLoading())
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAssetData::Serialize: Checking if NeedsAutoloading."));
		if (NeedsAutoLoading())
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::Serialize: Loading media asset data (%s - %u)"), *GetParentMediaName(),  GetParentMediaId());
			LoadMediaAssetData(true);
			FExternalReadCallback ExternalReadCallback = [this](double RemainingTime)
			{
				return this->ExternalReadCallback((RemainingTime));
			};
			Ar.AttachExternalReadDependency(ExternalReadCallback);
		}
	}
}

bool UAkMediaAssetData::ExternalReadCallback(double RemainingTime)
{
#if !WITH_EDITOR
	if (LoadingRequest)
	{
		if (RemainingTime < 0.0 && !LoadingRequest->PollCompletion())
		{
			return false;
		}
		else if (RemainingTime >= 0.0 && !LoadingRequest->WaitCompletion(RemainingTime))
		{
			return false;
		}
		return true;
	}
#endif
	return State == LoadState::Loaded || State == LoadState::Error;
}

bool UAkMediaAssetData::IsReadyForAsyncPostLoad() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return true;
	}

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("AkAudio")))
	{
		return true;
	}

	UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAssetData::IsReadyForAsyncPostLoad: Checking if NeedsAutoloading."));
	bool bNeedsAutoloading = NeedsAutoLoading();
	if (bNeedsAutoloading && State == LoadState::Unloaded)
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::IsReadyForAsyncPostLoad: NeedsAutoLoading returned true, but loadstate is unloaded. Media name : %s - %u"), *GetParentMediaName(),  GetParentMediaId());
		return false;
	}

	if (bNeedsAutoloading )
	{
		return State == LoadState::Loaded || State == LoadState::Error;
	}

	return true;
}

void UAkMediaAssetData::PostLoad()
{
	Super::PostLoad();
	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::PostLoad called for media %u."), GetParentMediaId());

#if !WITH_EDITOR
	if (LoadingRequest)
	{
		if (LoadingRequest->PollCompletion())
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::PostLoad: Deleting LoadingRequest for media %u"), GetParentMediaId());
			delete LoadingRequest;
			LoadingRequest = nullptr;
		}
	}
#endif
}

FString UAkMediaAssetData::StateToString() const
{
	switch (State)
	{
	case LoadState::Unloaded:
		return FString("Unloaded");
	case LoadState::Loading:
		return FString("Loading");
	case LoadState::Loaded:
		return FString("Loaded");
	case LoadState::Error:
	default:
		return FString("Error");
	}
}

void UAkMediaAssetData::LoadMediaAssetData(bool LoadAsync)
{
	if (!FAkAudioDevice::IsAudioAllowed() || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	if (Language != "SFX")
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::LoadMediaAssetData: Loading localized media data for media asset %s - %u for language %s."), *GetParentMediaName(), GetParentMediaId(), *Language);
	}

	if (State == LoadState::Loading)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::LoadMediaAssetData: Already have a loading request for Id %u. State is %s."), GetParentMediaId(), *StateToString());
		return;
	}

	UAkMediaAsset* ParentMediaAsset = Cast<UAkMediaAsset>(GetOuter());
	for (auto DataChunk : DataChunks)
	{
		if (IsStreamed && !DataChunk.IsPrefetch)
		{
			FAkUnrealIOHook::AddStreamingMedia(ParentMediaAsset);
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::LoadAndSetMedia: Added streaming media %u to IO Hook."), GetParentMediaId());
		}
	}

	auto* MediaMemoryManager = AkMediaMemoryManager::Get();
	if (DataChunks.Num() > 0 && ParentMediaAsset && MediaMemoryManager)
	{
		// Avoid loading the streamed chunk
		if (!IsStreamed || DataChunks[0].IsPrefetch)
		{
			if (!DataChunks[0].Data.IsValid())
			{
				UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::LoadMediaAssetData: DataChunk is invalid in media asset  %s - %u."), *GetParentMediaName(), GetParentMediaId());
				State = LoadState::Error;
				return;
			}

			int64 MediaSize = DataChunks[0].Data->GetBulkDataSize();
#if !WITH_EDITOR
			// Some platforms do not allow cancelling a streaming request, so the callback will always be called. There is a possibility the GC could decide to delete the object
			// before the transfer is complete. Without a way to cancel it, we would need to block on its completion before allowing the GC to delete the object. 
			// Instead, add this object to the root, preventing it from being garbage collected while a streaming request is in flight. This removes the need to wait on the
			// streaming request's completion in the destroy phase.
			BulkDataRequestCompletedCallback LoadAsyncCompleted = [this](bool bWasCancelled, ReadRequestArgumentType* ReadRequest)
			{
				if (IsEngineExitRequested())
				{
					RemoveFromRoot();
					return;
				}
				BulkDataStreamingRequestCompleted(bWasCancelled, ReadRequest);
				RemoveFromRoot();
			};

			if (DataChunks[0].Data->CanLoadFromDisk() && LoadAsync)
			{
				uint8* TempReadMediaMemory;
				const bool bMediaAlreadyInMemory = MediaMemoryManager->RequestMediaMemory(ParentMediaAsset->MediaName, ParentMediaAsset->Id, MediaSize, UseDeviceMemory, TempReadMediaMemory);
				if (!bMediaAlreadyInMemory)
				{
					if (TempReadMediaMemory)
					{
						State = LoadState::Loading;
						AddToRoot();
						UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::LoadMediaAssetData: Creating streaming request for %u"), ParentMediaAsset->Id);
						LoadingRequest = DataChunks[0].Data->CreateStreamingRequest(EAsyncIOPriorityAndFlags::AIOP_High, &LoadAsyncCompleted, TempReadMediaMemory);
					}
					else
					{
						State = LoadState::Error;
					}
				}
				else
				{
					UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::LoadMediaAssetData: Media already loaded for %u"), GetParentMediaId());
					State = LoadState::Loaded;
					LoadedMediaData = TempReadMediaMemory;
					ParentMediaAsset->bIsMediaSet = true;
				}
			}
			else
#endif
			{
				bool bMediaAlreadyInMemory = MediaMemoryManager->RequestMediaMemory(ParentMediaAsset->MediaName, GetParentMediaId(), MediaSize, UseDeviceMemory, LoadedMediaData);
				State = LoadState::Loaded;
				if (!bMediaAlreadyInMemory)
				{
					if (LoadedMediaData)
					{
						auto BulkMediaDataSize = DataChunks[0].Data->GetBulkDataSize();
						DataChunks[0].Data->GetCopy((void**)&LoadedMediaData, false);
						MediaMemoryManager->SetMedia(ParentMediaAsset->MediaName, GetParentMediaId(), LoadedMediaData, MediaSize);
						ParentMediaAsset->bIsMediaSet = true;
					}
					else
					{
						State = LoadState::Error;
					}
				}
				else
				{
					ParentMediaAsset->bIsMediaSet = true;
				}
			}
		}
		else
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::LoadMediaAssetData: Streaming media %u has no prefetch chunk. It is considered loaded."), GetParentMediaId());
			State = LoadState::Loaded;
		}

	}
	else
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::LoadMediaAssetData: Skipping media load. DataChunks.Num()=%u; LoadedMediaData=%u; Parent=%u; MediaMemoryManager=%u"), DataChunks.Num(), LoadedMediaData, ParentMediaAsset, MediaMemoryManager);
		State = LoadState::Error;
	}
}

#if !WITH_EDITOR
void UAkMediaAssetData::BulkDataStreamingRequestCompleted(bool bWasCancelled, ReadRequestArgumentType* ReadRequest)
{
	if (bWasCancelled)
	{
		auto* MediaMemoryManager = AkMediaMemoryManager::Get();
		auto MediaID = GetParentMediaId();
		if (MediaMemoryManager && MediaID != AK_INVALID_UNIQUE_ID)
		{
			MediaMemoryManager->ReleaseMediaMemory(MediaID);
		}
		State = LoadState::Error;
		UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::BulkDataStreamingRequestCompleted: Bulk data streaming request for Media ID %u was cancelled. Media will be unavailable."), GetParentMediaId());
	}
	else
	{
		LoadedMediaData = ReadRequest->GetReadResults();
		State = LoadState::Loaded;
		UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::BulkDataStreamingRequestCompleted: Finished loading Id %u. State is %s"), GetParentMediaId(), *StateToString());

		auto* MediaMemoryManager = AkMediaMemoryManager::Get();
		if (MediaMemoryManager)
		{
			if (!DataChunks[0].Data.IsValid())
			{
				State = LoadState::Error;
				UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::BulkDataStreamingRequestCompleted: DataChunk for Media ID %u is invalid. Media will be unavailable."), GetParentMediaId());
				return;
			}
			int64 MediaSize = DataChunks[0].Data->GetBulkDataSize();
			MediaMemoryManager->SetMedia(GetParentMediaName(), GetParentMediaId(), LoadedMediaData, MediaSize);
			if (UAkMediaAsset* ParentMediaAsset = Cast<UAkMediaAsset>(GetOuter()))
			{
				ParentMediaAsset->bIsMediaSet = true;
			}
		}
		else
		{
			UE_LOG(LogAkAudio, Error, TEXT("Could not get Media MemoryManager. Media %s - %u will not be set"), *GetParentMediaName(), GetParentMediaId());
		}

	}
}
#endif

void UAkMediaAssetData::UnloadMediaAssetData()
{
	if (!FAkAudioDevice::IsAudioAllowed() || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	
	AkMediaMemoryManager* MediaMemoryManager = AkMediaMemoryManager::Get();
	UAkMediaAsset* ParentMediaAsset = Cast<UAkMediaAsset>(GetOuter());
	uint32 MediaID = ParentMediaAsset ? ParentMediaAsset->Id : AK_INVALID_UNIQUE_ID;
	if (ParentMediaAsset)
	{
		if (ParentMediaAsset->bIsMediaSet)
		{
			if (MediaMemoryManager && MediaID != AK_INVALID_UNIQUE_ID && ParentMediaAsset->GetMediaAssetData() == this)
			{
				UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::UnloadMediaAssetData: Unsetting media %u. Media Asset data is at %p"), MediaID, this);
				const bool bMediaWasFreed = MediaMemoryManager->ReleaseMediaMemory(MediaID);
				if (bMediaWasFreed)
				{
					LoadedMediaData = nullptr;
					State = LoadState::Unloaded;
				}
			}
			else
			{
				UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::UnloadMediaAssetData: Could not unload media %s."), *GetOutermost()->GetName());
				State = LoadState::Error;
				LoadedMediaData = nullptr;
			}
		}
	}
	else if (LoadedMediaData != nullptr)
	{
		bool bMediaWasFreed = false;
		if (MediaMemoryManager && MediaID != AK_INVALID_UNIQUE_ID )
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::UnloadMediaAssetData: MediaAssetData %s has no valid parent, but memory was still set. Unsetting media %u. Media Asset data is at %p"), 
				*GetOutermost()->GetName(), MediaID, this);
			bMediaWasFreed = MediaMemoryManager->ReleaseMediaMemory(MediaID);
		}

		if (bMediaWasFreed)
		{
			State = LoadState::Error;
			LoadedMediaData = nullptr;
		}
	}
}

void UAkMediaAssetData::BeginDestroy()
{
	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData::BeginDestroy called for media data %s."), *GetParentMediaName());

	if (IsStreamed)
	{
		FAkUnrealIOHook::RemoveStreamingMedia(Cast<UAkMediaAsset>(GetOuter()));
	}

	if (IsCurrentWwisePlatform(AssetPlatform))
	{
		UnloadMediaAssetData();
	}

#if !WITH_EDITOR
	if (LoadingRequest)
	{
		// If we still have a request at this point, we need to free it even if it's not finished.
		// It is however forbidden to delete a pending request, so we need to wait for its completion.
		// We do not want to block the thread calling BeginDestroy, so we do it in a background task.
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [LoadRequest = LoadingRequest, MediaID = GetParentMediaId()]()
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAssetData: Deleting LoadingRequest for media %u in a background task created in BeginDestroy"), MediaID);
			LoadRequest->WaitCompletion();
			delete LoadRequest;
		});
	}
#endif

	Super::BeginDestroy();
}

bool UAkMediaAssetData::NeedsAutoLoading() const
{
	if (!FAkAudioDevice::IsAudioAllowed() || !FModuleManager::Get().IsModuleLoaded(TEXT("AkAudio")))
	{
		// If AkAudio module is not yet loaded, parent media asset will add itself to the delay load list,
		// and we will eventually be loaded.
		return false;
	}

	if (!IsCurrentWwisePlatform(AssetPlatform))
	{
		const FString MediaName = GetParentMediaName();
		const uint32 MediaId = GetParentMediaId();
		const FString IniName = FPlatformProperties::IniPlatformName();
		#if !WITH_EDITORONLY_DATA
			UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::NeedsAutoLoading: Platform %s for media asset data appears to be invalid for current platform (%s). Media name:  %s - %u"), *AssetPlatform, *IniName, *MediaName, MediaId);
		#else
			UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAssetData::NeedsAutoLoading: Not loading asset data for platform %s as it is not the current platform %s. Media name:  %s- %u"), *AssetPlatform, *IniName, *MediaName, MediaId);
		#endif
		return false;
	}

	if (DataChunks.Num() > 0)
	{

		bool bIsSFX = Language == TEXT("SFX");
		bool bIsCurrentAudioCulture = false;

		FString CurrentAudioCulture = TEXT("");
		if (!bIsSFX)
		{
			if (auto AkAudioDevice = FAkAudioDevice::Get())
			{
				CurrentAudioCulture = AkAudioDevice->GetCurrentAudioCulture();
				bIsCurrentAudioCulture = Language == CurrentAudioCulture;
			}
		}

		UAkMediaAsset *ParentMediaAsset = Cast<UAkMediaAsset>(GetOuter());
		FString MediaName = GetParentMediaName();
		uint32 MediaId = GetParentMediaId();
		if (!ParentMediaAsset)
		{
			UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAssetData::NeedsAutoLoading: Asset data will not be loaded because parent is not valid. Media name:  %s- %u"),
				   *MediaName, MediaId);
		}
		if (!bIsSFX && !bIsCurrentAudioCulture)
		{
			UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAssetData::NeedsAutoLoading: Asset data will not be loaded because it is not SFX and language does not match current language (%s). Media Language : %s. Media name: %s - %u."), *CurrentAudioCulture, *Language, *MediaName, MediaId);
		}
		if (ParentMediaAsset && (ParentMediaAsset->AutoLoad && (bIsSFX || bIsCurrentAudioCulture)))
		{
			UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAssetData::NeedsAutoLoading: Asset needs autoloading. AutoLoad = %d | IsSFX = %d | ISCurrentAudioCulture = %d.  Media name: %s - %u "), ParentMediaAsset->AutoLoad, bIsSFX, bIsCurrentAudioCulture, *MediaName, MediaId);
			return true;
		}
	}

	return false;
}

void UAkMediaAsset::Serialize(FArchive& Ar)
{
	SerializeHasBeenCalled = true;
	Super::Serialize(Ar);

	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::Serialize: Media is (%s - %u). ForceAutoLoad = %d | AutoLoad = %d | LoadFromSerialize = %d"), *MediaName,  Id, ForceAutoLoad, AutoLoad, LoadFromSerialize);

	if (Ar.IsLoading())
	{
		ForceAutoLoad = AutoLoad || LoadFromSerialize;
	}

#if WITH_EDITORONLY_DATA
	if (Ar.IsFilterEditorOnly())
	{
		if (Ar.IsSaving() && !Ar.CookingTarget()->IsServerOnly() && AkUnrealHelper::IsUsingEventBased())
		{
			TArray<FString> AvailableWwisePlatforms;
			MediaAssetDataPerPlatform.GenerateKeyArray(AvailableWwisePlatforms);
			UAkPlatformInfo* CurrentPlatformInfo = UAkPlatformInfo::GetAkPlatformInfo(Ar.CookingTarget()->IniPlatformName());
			FString WwisePlatformName;
			if(LIKELY(CurrentPlatformInfo))
			{
				WwisePlatformName = CurrentPlatformInfo->GetWwiseBankPlatformName(AvailableWwisePlatforms);
			}
			else
			{
				WwisePlatformName = Ar.CookingTarget()->IniPlatformName();
			}
			const auto CurrentMediaData = MediaAssetDataPerPlatform.Find(WwisePlatformName);
			CurrentMediaAssetData = CurrentMediaData ? *CurrentMediaData : nullptr;
			UE_CLOG(!CurrentMediaAssetData, LogAkAudio, Warning, TEXT("UAkMediaAsset::Serialize: Cound not find platform %s asset data for media asset %s. Media will not be available in packaged game."), *WwisePlatformName , *GetOutermost()->GetName());
		}
		Ar << CurrentMediaAssetData;
	}
	else
	{
		Ar << MediaAssetDataPerPlatform;
	}
#else
	Ar << CurrentMediaAssetData;
#endif
	if (!FModuleManager::Get().IsModuleLoaded(TEXT("AkAudio")) && ForceAutoLoad)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::Serialize: %s - %u: AkAudio module not initialized, delaying media load"), *MediaName, Id);
		FAkAudioDevice::DelayMediaLoad(this);
		return;
	}

}

#if WITH_EDITOR
#include "Async/Async.h"
void UAkMediaAsset::Reset()
{
	UnloadMedia();

	bool bChanged = false;
	if (MediaAssetDataPerPlatform.Num() > 0 || !MediaName.IsEmpty() || CurrentMediaAssetData != nullptr)
	{
		bChanged = true;
	}

	MediaAssetDataPerPlatform.Empty();
	MediaName.Empty();
	CurrentMediaAssetData = nullptr;
	
	if (bChanged)
	{
		AsyncTask(ENamedThreads::GameThread, [this] {
			MarkPackageDirty();
			});
	}
}

UAkMediaAssetData* UAkMediaAsset::FindOrAddMediaAssetData(const FString& Platform)
{
	auto PlatformData = MediaAssetDataPerPlatform.Find(Platform);
	if (PlatformData)
	{
		return *PlatformData;
	}

	auto NewPlatformData = NewObject<UAkMediaAssetData>(this);
	NewPlatformData->Language = this->Language;
	MediaAssetDataPerPlatform.Add(Platform, NewPlatformData);
	return NewPlatformData;
}
#endif

void UAkMediaAsset::LoadMedia(bool bFromSerialize /*= false*/)
{
	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::LoadMedia: Loading media %s - %u"), *MediaName, Id);

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("AkAudio")))
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::LoadMedia: %s - %u: AkAudio module not initialized, delaying media load"), *MediaName, Id);
		FAkAudioDevice::DelayMediaLoad(this);
		return;
	}
	
	if (auto AkAudioDevice = FAkAudioDevice::Get())
	{
		auto CurrentCulture = AkAudioDevice->GetCurrentAudioCulture();
		if (Language != TEXT("SFX") && CurrentCulture != Language)
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::LoadMedia: Not calling LoadAndSetMedia because current language could not be determined. Media Name : %s. Media ID : %u. Media language : %s."), *MediaName, Id, *Language);
			return;
		}
	}

	if (UNLIKELY(bFromSerialize && !SerializeHasBeenCalled))
	{
		LoadFromSerialize = true;
	}
	else
	{
		LoadAndSetMedia(true);
	}
}

bool UAkMediaAsset::IsReadyForAsyncPostLoad() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return true;
	}
	if (auto MediaAssetData = GetMediaAssetData())
	{
		return MediaAssetData->IsReadyForAsyncPostLoad();
	}

	return true;
}

FAkMediaDataChunk const* UAkMediaAsset::GetStreamedChunk() const
{
	auto MediaAssetData = GetMediaAssetData();
	if (!MediaAssetData || MediaAssetData->DataChunks.Num() <= 0)
	{
		return nullptr;
	}

	if (!MediaAssetData->DataChunks[0].IsPrefetch)
	{
		return &MediaAssetData->DataChunks[0];
	}

	if (MediaAssetData->DataChunks.Num() >= 2)
	{
		return &MediaAssetData->DataChunks[1];
	}

	return nullptr;
}

UAkMediaAssetData::LoadState UAkMediaAsset::GetLoadState() const
{
	if (const UAkMediaAssetData* MediaAssetData = GetMediaAssetData())
	{
		return MediaAssetData->GetLoadState();
	}
	return UAkMediaAssetData::LoadState::Error;
}

FString UAkMediaAsset::GetLoadStateString() const
{
	if (const UAkMediaAssetData* MediaAssetData = GetMediaAssetData())
	{
		return MediaAssetData->StateToString();
	}
	return TEXT("Error");
}

bool UAkMediaAsset::ExternalReadCallback(double RemainingTime)
{
	if (UAkMediaAssetData* MediaAssetData = GetMediaAssetData())
	{
		return MediaAssetData->ExternalReadCallback(RemainingTime);
	}
	return true;
}

void UAkMediaAsset::LoadAndSetMedia(bool bLoadAsync)
{
	auto MediaAssetData = GetMediaAssetData();
	if (!MediaAssetData || MediaAssetData->DataChunks.Num() <= 0)
	{
#if WITH_EDITOR
		ForceAutoLoad = true;
#endif
		return;
	}

	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::LoadAndSetMedia: Id is %u. Name is %s. State is %s."), Id, *MediaName, *MediaAssetData->StateToString());
	for (auto& DataChunk : MediaAssetData->DataChunks)
	{
		if (MediaAssetData->IsStreamed && !DataChunk.IsPrefetch)
		{
			FAkUnrealIOHook::AddStreamingMedia(this);
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::LoadAndSetMedia: Added streaming media."));
		}
	}

	if (MediaAssetData->IsStreamed && !MediaAssetData->DataChunks[0].IsPrefetch)
	{
		// This media is streaming with no prefetch. Nothing else to do here.
		return;
	}

#if !WITH_EDITOR
	if (!MediaAssetData->DataChunks[0].Data.IsValid() || MediaAssetData->DataChunks[0].Data->GetBulkDataSize() <= 0)
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkMediaAssetData::LoadAndSetMedia: DataChunk for Media %s - %u is invalid. Media will be unavailable."), *MediaName, Id);
		return;
	}
#endif

	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::LoadAndSetMedia: Loading media asset data. Media is %s - %u. State is %s."),*MediaName,  Id, *MediaAssetData->StateToString());
	MediaAssetData->LoadMediaAssetData(bLoadAsync);
}


void UAkMediaAsset::UnloadMedia()
{
	UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::UnloadMedia: Id is %u. Name is %s."), Id, *MediaName);
	auto MediaAssetData = GetMediaAssetData();
	if (!MediaAssetData)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("UAkMediaAsset::UnloadMedia: Could not get MediaAssetData, returning. Id is %u. Name is %s."), Id, *MediaName);
		return;
	}

	if (MediaAssetData->IsStreamed)
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAsset::UnloadMedia:Removing media from IO hook. Id is %u. Name is %s."), Id, *MediaName);
		FAkUnrealIOHook::RemoveStreamingMedia(this);
	}

	if (bIsMediaSet)
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAsset::UnloadMedia: Unloading MediaAssetData. Id is %u. Name is %s."), Id, *MediaName);
		MediaAssetData->UnloadMediaAssetData();
		bIsMediaSet = MediaAssetData->IsLoaded();
	}
	else
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkMediaAsset::UnloadMedia: Not unloading because media is not set. Id is %u. Name is %s."), Id, *MediaName);
	}
}

UAkMediaAssetData* UAkMediaAsset::GetMediaAssetData() const
{
#if !WITH_EDITORONLY_DATA
	return CurrentMediaAssetData;
#else
	const FString RunningPlatformName(FPlatformProperties::IniPlatformName());
	if (auto PlatformMediaData = MediaAssetDataPerPlatform.Find(RunningPlatformName))
	{
		return *PlatformMediaData;
	}

	return nullptr;
#endif
}

UAkExternalMediaAsset::UAkExternalMediaAsset()
{
	AutoLoad = false;
}

void UAkExternalMediaAsset::Serialize(FArchive& Ar)
{
	if (Id == AK_INVALID_UNIQUE_ID)
	{
		Id = FAkAudioDevice::GetIDFromString(GetName());
	}
	Super::Serialize(Ar);
}

TTuple<void*, int64> UAkExternalMediaAsset::GetExternalSourceData()
{
	auto MediaAssetData = GetMediaAssetData();

	if (MediaAssetData && MediaAssetData->DataChunks.Num() > 0 && MediaAssetData->DataChunks[0].Data.IsValid())
	{
		if (GetLoadState() == UAkMediaAssetData::LoadState::Unloaded)
		{
			UE_LOG(LogAkAudio, Verbose, TEXT("UAkExternalMediaAsset::GetExternalSourceData: Loading media data for media %s - %u."), *MediaName, Id);
			LoadAndSetMedia(false);
		}
		if (GetLoadState() == UAkMediaAssetData::LoadState::Error)
		{
			UE_LOG(LogAkAudio, Error, TEXT("UAkExternalMediaAsset::GetExternalSourceData: There was an error loading media data for media %s - %u. Media will be unavailable."), *MediaName, Id);
		}
		else if (GetLoadState() == UAkMediaAssetData::LoadState::Loading)
		{
			UE_LOG(LogAkAudio, Warning, TEXT("UAkExternalMediaAsset::GetExternalSourceData: Media data is still loading for media %s - %u. Media will not be played."), *MediaName, Id);
		}
		auto result = MakeTuple(static_cast<void*>(MediaAssetData->LoadedMediaData), MediaAssetData->DataChunks[0].Data->GetBulkDataSize());
		return result;
	}
	else
	{
		UE_LOG(LogAkAudio, Error, TEXT("UAkExternalMediaAsset::GetExternalSourceData: MediaAssetData or DataChunks for Media %s - %u are invalid. Media will be unavailable."), *MediaName, Id);
	}

	return {};
}

void UAkExternalMediaAsset::AddPlayingIDAndPinInGarbageCollector(uint32 EventID, uint32 PlayingID)
{
	PinInGarbageCollector(PlayingID);
	auto& PlayingIDArray = ActiveEventToPlayingIDMap.FindOrAdd(EventID);
	PlayingIDArray.Add(PlayingID);
}

bool UAkExternalMediaAsset::HasActivePlayingIDs()
{
	if (auto* AudioDevice = FAkAudioDevice::Get())
	{
		for (auto ActiveEventToPlayingIDs : ActiveEventToPlayingIDMap)
		{
			uint32 EventID = ActiveEventToPlayingIDs.Key;
			for (auto PlayingID : ActiveEventToPlayingIDs.Value)
			{
				if (AudioDevice->IsPlayingIDActive(EventID, PlayingID))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void UAkExternalMediaAsset::BeginDestroy()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		Super::BeginDestroy();
		return;
	}
	UE_LOG(LogAkAudio, Verbose, TEXT("UAkExternalMediaAsset::BeginDestroy called for media %s."), *MediaName);

	if (auto* AudioDevice = FAkAudioDevice::Get())
	{
		for (auto ActiveEventToPlayingIDs : ActiveEventToPlayingIDMap)
		{
			uint32 EventID = ActiveEventToPlayingIDs.Key;
			for (auto PlayingID : ActiveEventToPlayingIDs.Value)
			{
				if (AudioDevice->IsPlayingIDActive(EventID, PlayingID))
				{
					UE_LOG(LogAkAudio, Warning, TEXT("UAkExternalMediaAsset::BeginDestroy: Stopping PlayingID %u because media file %s is being unloaded."), PlayingID, *GetName());
					AudioDevice->StopPlayingID(PlayingID);
				}
			}
		}
	}
	Super::BeginDestroy();
}

bool UAkExternalMediaAsset::IsReadyForFinishDestroy()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return Super::IsReadyForFinishDestroy();
	}
	if (HasActivePlayingIDs())
	{
		if (auto* AudioDevice = FAkAudioDevice::Get())
		{
			AudioDevice->Update(0.0);
			return false;
		}
	}
	return Super::IsReadyForFinishDestroy();
}

void UAkExternalMediaAsset::PinInGarbageCollector(uint32 PlayingID)
{
	const auto PinCount = TimesPinnedToGC.Increment();
	if (PinCount == 1)
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkExternalMediaAsset : Pinning Media %s in Garbage Collector."), *MediaName);
		AddToRoot();
		if (CurrentMediaAssetData)
		{
			CurrentMediaAssetData->AddToRoot();
		}
	}
	UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkExternalMediaAsset::PinInGarbageCollector: Media %s TimesPinnedToGC : %d."), *MediaName, PinCount);
	if (auto* AudioDevice = FAkAudioDevice::Get())
	{
		AudioDevice->AddToPinnedMediasMap(PlayingID, this);
	}
}

void UAkExternalMediaAsset::UnpinFromGarbageCollector(uint32 PlayingID)
{
	const auto PinCount = TimesPinnedToGC.Decrement();
	UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkExternalMediaAsset::UnpinFromGarbageCollector: Media %s TimesPinnedToGC : %d."), *MediaName, PinCount);
	if (PinCount == 0)
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkExternalMediaAsset::UnpinFromGarbageCollector : Unpinning Media %s from Garbage Collector."), *MediaName);
		RemoveFromRoot();
		if (CurrentMediaAssetData)
		{
			CurrentMediaAssetData->RemoveFromRoot();
		}
	}
}

bool AkMediaAssetHelpers::IsMediaReady(const TArray<UAkMediaAsset*>& InMediaList)
{
	FString CurrentAudioCulture = "Unknown";

	if (auto* AudioDevice = FAkAudioDevice::Get())
	{
		CurrentAudioCulture = AudioDevice->GetCurrentAudioCulture();
	}

	for (UAkMediaAsset* MediaAsset : InMediaList)
	{
		if (IsValid(MediaAsset))
		{
			//Any media that is meant to be in memory should either be loaded or loading
			switch (MediaAsset->GetLoadState())
			{
			case UAkMediaAssetData::LoadState::Loading:
				return false;
				break;
			case UAkMediaAssetData::LoadState::Error:
				UE_LOG(LogAkAudio, Warning, TEXT("AkMediaAssetHelpers::IsMediaReady : Media failed to load or could not obtain LoadState for media asset %s. Media language is %s."), *MediaAsset->MediaName, *MediaAsset->Language);
			case UAkMediaAssetData::LoadState::Unloaded:
			case UAkMediaAssetData::LoadState::Loaded:
			default:
				continue;
			}
		}
	}
	return true;
}
