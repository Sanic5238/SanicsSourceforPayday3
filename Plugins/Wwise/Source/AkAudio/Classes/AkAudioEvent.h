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

#include "AkAssetBase.h"
#include "Serialization/BulkData.h"
#include "AkAudioEvent.generated.h"

class UAkGroupValue;
class UAkMediaAsset;
class UAkAudioBank;
class UAkAuxBus;
class UAkTrigger;
struct FStreamableHandle;
class UAkAssetPlatformData;

UCLASS(BlueprintType)
class AKAUDIO_API UAkAudioEvent : public UAkAssetBase
{
	GENERATED_BODY()

public:
	/** Maximum attenuation radius for this event */
	UFUNCTION(BlueprintGetter, Category = "AkAudioEvent")
	float GetMaxAttenuationRadius() const;

	/** Whether this event is infinite (looping) or finite (duration parameters are valid) */
	UFUNCTION(BlueprintGetter, Category = "AkAudioEvent")
	bool GetIsInfinite() const;

	/** Minimum duration */
	UFUNCTION(BlueprintGetter, Category = "AkAudioEvent")
	float GetMinimumDuration() const;
	void SetMinimumDuration(float value);

	/** Maximum duration */
	UFUNCTION(BlueprintGetter, Category = "AkAudioEvent")
	float GetMaximumDuration() const;
	void SetMaximumDuration(float value);

	UPROPERTY(VisibleAnywhere, Category = "AkAudioEvent")
	TMap<FString, UAkAssetPlatformData*> LocalizedPlatformAssetDataMap;

	UPROPERTY(EditAnywhere, Category = "AkAudioEvent")
	UAkAudioBank* RequiredBank = nullptr;

	UAkAudioBank* LastRequiredBank = nullptr;

#if WITH_EDITOR	
	void UpdateRequiredBanks();
	void ClearRequiredBank();

	UAkAudioBank* UndoCompareBank = nullptr;
	void PostEditUndo() override;
	void PreEditUndo() override;
#if UE_4_25_OR_LATER
	void PreEditChange(FProperty* PropertyAboutToChange) override;
#else
	void PreEditChange(UProperty* PropertyAboutToChange) override;
#endif
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	UAkAssetPlatformData* CurrentLocalizedPlatformData = nullptr;

	/** Maximum attenuation radius for this event */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetMaxAttenuationRadius, Category = "AkAudioEvent")
	float MaxAttenuationRadius = .0f;

	/** Whether this event is infinite (looping) or finite (duration parameters are valid) */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetIsInfinite, Category = "AkAudioEvent")
	bool IsInfinite = false;

	/** Minimum duration */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetMinimumDuration, Category = "AkAudioEvent")
	float MinimumDuration = .0f;

	/** Maximum duration */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetMaximumDuration, Category = "AkAudioEvent")
	float MaximumDuration = .0f;

public:
	void LoadBank() override;
	void BeginDestroy() override;
	virtual bool IsLocalized() const override ;

	bool IsReadyForAsyncPostLoad() const override;

	bool IsLocalizationReady() const;

	bool SwitchLanguage(const FString& newAudioCulture);

	void PinInGarbageCollector(uint32 PlayingID);
	void UnpinFromGarbageCollector(uint32 PlayingID);

#if WITH_EDITOR
	UAkAssetData* FindOrAddAssetData(const FString& platform, const FString& language) override;
	void Reset(TArray<FAssetData>& InOutAssetsToDelete) override;
	bool NeedsRebuild(const TSet<FString>& PlatformsToBuild, const TSet<FString>& LanguagesToBuild, const ISoundBankInfoCache* SoundBankInfoCache) const override;
	bool UndoFlag = false;
#endif

	friend class AkEventBasedIntegrationBehavior;

protected:
	UAkAssetData* CreateAssetData(UObject* parent) const override;
	UAkAssetData* GetAssetData() const override;

private:
	bool LoadLocalizedBank(const FString& AudioCulture);
	bool LoadLocalizedMedia();
	bool LoadLocalizedData(const FString& AudioCulture, const bool& bCalledFromLoad = false);
	void UnloadLocalizedData();
	void AssetBaseLoadBank();

private:
	FThreadSafeCounter TimesPinnedToGC;
};
