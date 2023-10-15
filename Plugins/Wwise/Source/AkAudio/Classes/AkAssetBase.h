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

#include "AkAudioDevice.h"
#include "AkAudioType.h"
#include "AkAssetData.h"
#include "Serialization/BulkData.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/Object.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "AkAssetBase.generated.h"

class ISoundBankInfoCache;
class UAkMediaAsset;
struct FStreamableHandle;

UCLASS()
class AKAUDIO_API UAkAssetBase : public UAkAudioType
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "AkAssetBase")
	UAkAssetPlatformData* PlatformAssetData = nullptr;

public:
	void FinishDestroy() override;

	virtual void LoadBank() override;
	virtual void UnloadBank();
	
#if WITH_EDITOR ||WITH_EDITORONLY_DATA
	template <typename T>
	void FindObjectsReferencingThis( UClass * MatchClass, TSet<TSoftObjectPtr<T>>& OutFoundObjectSet)
	{
		TArray<FAssetIdentifier> Referencers;
		auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
#if UE_4_26_OR_LATER
		AssetRegistryModule.Get().GetReferencers(FName(*(this->GetOuter()->GetPathName())), Referencers, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Hard);
#else
		AssetRegistryModule.Get().GetReferencers(FName(*(this->GetOuter()->GetPathName())), Referencers, EAssetRegistryDependencyType::All);
#endif
		if (Referencers.Num() > 0)
		{
			FARFilter Filter;
			for (FAssetIdentifier& Referencer : Referencers)
			{
				Filter.PackageNames.Add(Referencer.PackageName);
			}
			TArray<FAssetData> Assets;
#if UE_5_1_OR_LATER
			Filter.ClassPaths.Add(MatchClass->GetClassPathName());
#else
			Filter.ClassNames.Add(MatchClass->StaticClass()->GetFName());
#endif
			AssetRegistryModule.Get().GetAssets(Filter, Assets);
			for (FAssetData& AssetData : Assets)
			{
				OutFoundObjectSet.Add(TSoftObjectPtr<T>(AssetData.ToSoftObjectPath()));
			}
		}
	}
#endif

#if WITH_EDITOR
	template<typename T>
	T* FindOrAddAssetDataTyped(const FString& Platform, const FString& Language)
	{
		return Cast<T>(FindOrAddAssetData(Platform, Language));
	}

	virtual UAkAssetData* FindOrAddAssetData(const FString& Platform, const FString& Language);
	virtual void GetMediaList(TArray<TSoftObjectPtr<UAkMediaAsset>>& MediaList) const;

	virtual bool NeedsRebuild(const TSet<FString>& PlatformsToBuild, const TSet<FString>& LanguagesToBuild, const ISoundBankInfoCache* SoundBankInfoCache) const;

	void Reset(TArray<FAssetData>& InOutAssetsToDelete) override;


	FCriticalSection BulkDataWriteLock;
#endif

	virtual bool IsLocalized() const {return false;}
	void GetPreloadDependencies(TArray<UObject*>& OutDeps);

protected:
	virtual UAkAssetData* CreateAssetData(UObject* Parent) const;
	virtual UAkAssetData* GetAssetData() const;
	bool bDelayLoadAssetData = false;

	UAkAssetData* InternalFindOrAddAssetData(UAkAssetPlatformData* Data, const FString& Platform, UObject* Parent);

protected:
#if WITH_EDITOR
	FCriticalSection AssetDataLock;
#endif
};

using SwitchLanguageCompletedFunction = TFunction<void(bool)>;

