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


#pragma once
#include "Engine/EngineTypes.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"

DECLARE_MULTICAST_DELEGATE(OnAllMediaFreedDelegate);

class UAkMediaAsset;

struct FAkMediaMemoryEntry
{
	uint8* MediaMemory;
	int32 RefCount;
	int64 DataSize;
	bool bUseDeviceMemory;
};

class AkMediaMemoryManager : public FRunnable
{
public:
	AkMediaMemoryManager();
	~AkMediaMemoryManager();

	static AkMediaMemoryManager* Get();

	/** Request media memory. If the mediaID already requested memory, this will return the memory
	 *  that already was allocated, and increase a reference count.
	 *
	 * @param MediaID			MediaID requesting the memory.
	 * @param MediaSize			Size in bytes of the requested memory.
	 * @param UseDeviceMemory	True if this media ID required device memory, false if not.
	 * @param out_MediaMemory	Media memory that was allocated. Can be nullptr if something went wrong.
	 * @return					False if the memory was newly allocated, true if it was already existing
	 */
	bool RequestMediaMemory(FString MediaName, uint32 MediaId, int64 MediaSize, bool UseDeviceMemory, uint8*& out_MediaMemory);

	/** Indicate this mediaID does not need the memory, allowing the manager to free it once Wwise
	 *  does not need it anymore.
	 *
	 * @param MediaID		MediaID that wants to release the memory
	 * @return Whether the memory was flagged to be freed. When returning false, this media is used elsewhere.
	 */
	bool ReleaseMediaMemory(uint32 MediaID);

	void SetMedia(FString MediaName, uint32 MediaId, uint8* LoadedMediaData, int64 MediaDataSize);

	// FRunnable interface
	uint32 Run() override;
	void Stop() override;

	OnAllMediaFreedDelegate OnAllPendingMediaFreed;
	bool HasPendingMediaToRemove();

private:
	uint8* AllocateMediaMemory(int64 DataSize, bool UseDeviceMemory);
	void FreeMediaMemory(uint8* MediaMemory, int64 DataSize, bool UseDeviceMemory);
	void CleanUpMedia();
	int GetNumberOfEntriesToRemove();

	static AkMediaMemoryManager* Instance;

	FRunnableThread* Thread;
	FThreadSafeBool bRunThread = true;
	FEvent* StartCleanUpEvent;

	FCriticalSection MediaIDToMemoryMapLock;
	TMap<uint32, FAkMediaMemoryEntry> MediaIDToMemoryMap;

	FCriticalSection EntriesToRemoveLock;
	TArray<uint32> EntriesToRemove;

	void FlagIDForRemoval(uint32 MediaID);
	void FlagIDsForRemoval(const TArray<uint32>& MediaIDs);
	uint32 GetNextIDToRemove();

};