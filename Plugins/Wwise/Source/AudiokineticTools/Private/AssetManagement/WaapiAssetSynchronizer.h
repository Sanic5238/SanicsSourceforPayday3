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

#include "AkWaapiClient.h"
#include "WwiseItemType.h"
#include "AssetRegistry/AssetData.h"

class WaapiAssetSynchronizer
{
public:
	void Init();
	void Uninit();

	static bool PauseAssetRegistryDelegates;

private:
	struct FRenameData {
		const bool IsFolder;
		const int NewPathDepth;
		const int OldPathDepth;
		const FGuid Id;
		const FAssetData AssetData;
		const FString OldPath;
		const FString NewPath;
	};

private:
	void subscribeWaapiCallbacks();
	void unsubscribeWaapiCallbacks();

	void onAssetRemoved(const FAssetData& RemovedAssetData);
	void onAssetRenamed(const FAssetData& NewAssetData, const FString& OldPath);
	void renameAssets(TArray<FRenameData>& assetsToRename);
	bool renameAsset(const FRenameData& RenameData);
	void undoAssetRenames(const TArray<FRenameData>& assetsToRename);
	void undoAssetRename(const FAssetData& NewAssetData, const FString& OldPath);

	void onRenamed(uint64_t Id, TSharedPtr<FJsonObject> Response);
	void onPreDeleted(uint64_t Id, TSharedPtr<FJsonObject> Response);
	void onChildAdded(uint64_t Id, TSharedPtr<FJsonObject> Response);
	void onChildRemoved(uint64_t Id, TSharedPtr<FJsonObject> Response);

	void assetUpdateTimerTick(float DeltaSeconds);

	static UClass* GetClassByName(const FString& stringAssetType);
	static  EWwiseItemType::Type GetTypeByName(const FString& stringAssetType);

private:
	FDelegateHandle projectLoadedHandle;
	FDelegateHandle connectionLostHandle;
	FDelegateHandle clientBeginDestroyHandle;

	FDelegateHandle assetAddedHandle;
	FDelegateHandle assetRemovedHandle;
	FDelegateHandle assetRenamedHandle;

	uint64 idRenamed = 0;
	uint64 idPreDeleted = 0;
	uint64 idChildAdded = 0;
	uint64 idChildRemoved = 0;

	TSet<FGuid> RenamesToIgnore;

	FDelegateHandle postEditorTickHandle;
	TSet<FGuid> assetsToDelete;
	TArray<FRenameData> assetsToRename;

	float deleteTimer = 0.f;
	float syncTimer = 0.f;
	const FString UnnamedStateGroupFolder = TEXT("<Unnamed State Group>");

};
