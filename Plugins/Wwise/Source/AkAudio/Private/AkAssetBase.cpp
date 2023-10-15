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

#include "AkAssetBase.h"
#include "AkMediaAsset.h"
#include "AkUnrealHelper.h"
#include "Platforms/AkPlatformInfo.h"
#include "IntegrationBehavior/AkIntegrationBehavior.h"
#include "AssetRegistry/AssetRegistryModule.h"

#if WITH_EDITOR
#include "TargetPlatform/Public/Interfaces/ITargetPlatform.h"
#include "ISoundBankInfoCache.h"
#endif

void UAkAssetBase::FinishDestroy()
{
	UnloadBank();
	Super::FinishDestroy();
}

void UAkAssetBase::LoadBank()
{
	Super::LoadBank();
	if (!bDelayLoadAssetData)
	{
		if (auto AssetData = GetAssetData())
		{
			AssetData->LoadAssetData();
		}
	}
}

void UAkAssetBase::UnloadBank()
{
	if (UAkAssetData* AssetData = GetAssetData())
	{
		AssetData->UnloadAssetData();
	}
}

UAkAssetData* UAkAssetBase::GetAssetData() const
{
	if (!PlatformAssetData)
		return nullptr;

#if WITH_EDITORONLY_DATA
	if (auto AssetData = PlatformAssetData->AssetDataPerPlatform.Find(FPlatformProperties::IniPlatformName()))
		return *AssetData;

	return nullptr;
#else
	return PlatformAssetData->CurrentAssetData;
#endif
}

UAkAssetData* UAkAssetBase::CreateAssetData(UObject* Parent) const
{
	return NewObject<UAkAssetData>(Parent);
}

#if WITH_EDITOR
UAkAssetData* UAkAssetBase::FindOrAddAssetData(const FString& Platform, const FString& Language)
{
	FScopeLock autoLock(&AssetDataLock);

	if (!PlatformAssetData)
	{
		PlatformAssetData = NewObject<UAkAssetPlatformData>(this);
	}

	return InternalFindOrAddAssetData(PlatformAssetData, Platform, this);
}

UAkAssetData* UAkAssetBase::InternalFindOrAddAssetData(UAkAssetPlatformData* Data, const FString& Platform, UObject* Parent)
{
	auto AssetData = Data->AssetDataPerPlatform.Find(Platform);
	if (AssetData)
		return *AssetData;

	auto NewAssetData = CreateAssetData(Parent);
	Data->AssetDataPerPlatform.Add(Platform, NewAssetData);
	return NewAssetData;
}

void UAkAssetBase::GetMediaList(TArray<TSoftObjectPtr<UAkMediaAsset>>& MediaList) const
{
	if (PlatformAssetData)
	{
		PlatformAssetData->GetMediaList(MediaList);
	}
}

bool UAkAssetBase::NeedsRebuild(const TSet<FString>& PlatformsToBuild, const TSet<FString>& LanguagesToBuild, const ISoundBankInfoCache* SoundBankInfoCache) const
{
	bool needsRebuild = false;

	if (PlatformAssetData)
	{
		needsRebuild |= PlatformAssetData->NeedsRebuild(PlatformsToBuild, TEXT("SFX"), ID, SoundBankInfoCache);
	}
	else
	{
		needsRebuild = true;
	}

	TArray<TSoftObjectPtr<UAkMediaAsset>> mediaList;
	GetMediaList(mediaList);

	for (auto& media : mediaList)
	{
		if (media.ToSoftObjectPath().IsValid() && !media.IsValid())
		{
			needsRebuild = true;
		}
	}

	return needsRebuild;
}

void UAkAssetBase::Reset(TArray<FAssetData>& InOutAssetsToDelete)
{
	if (PlatformAssetData != nullptr)
	{
		bChangedDuringReset = true;
	}
	PlatformAssetData = nullptr;

	// ALWAYS call Super::Reset() last, since it will check if things have been modified
	// before marking as dirty.
	Super::Reset(InOutAssetsToDelete);
}
#endif

void UAkAssetBase::GetPreloadDependencies(TArray<UObject*>& OutDeps)
{
	Super::GetPreloadDependencies(OutDeps);
	OutDeps.Add(PlatformAssetData);
}
