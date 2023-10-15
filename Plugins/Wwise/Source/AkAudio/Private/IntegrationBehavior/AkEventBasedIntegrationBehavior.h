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

#include "AkIntegrationBehavior.h"

class AkEventBasedIntegrationBehavior : public AkIntegrationBehavior
{
public:
	// AkAssetBase
	AKRESULT AkAssetData_LoadBank(UAkAssetData* AkAssetData) override;

	// AkAudioBank
	void AkAudioBank_Load(UAkAudioBank* AkAudioBank) override;
	void AkAudioBank_Unload(UAkAudioBank* AkAudioBank) override;

	// AkAudioDevice
	AKRESULT AkAudioDevice_ClearBanks(FAkAudioDevice* AkAudioDevice) override;
	AKRESULT AkAudioDevice_LoadInitBank(FAkAudioDevice* AkAudioDevice) override;
	bool AkAudioDevice_LoadAllFilePackages(FAkAudioDevice* AkAudioDevice) override;
	bool AkAudioDevice_UnloadAllFilePackages(FAkAudioDevice* AkAudioDevice) override;
	void AkAudioDevice_LoadAllReferencedBanks(FAkAudioDevice* AkAudioDevice) override;
	void AkAudioDevice_SetCurrentAudioCulture(const FString& NewWwiseLanguage) override;
	void AkAudioDevice_SetCurrentAudioCultureAsync(FAkAudioDevice* AkAudioDevice, const FString& NewWwiseLanguage, FSetCurrentAudioCultureAction* LatentAction) override;
	void AkAudioDevice_SetCurrentAudioCultureAsync(FAkAudioDevice* AkAudioDevice, const FString& NewWwiseLanguage, const FOnSetCurrentAudioCultureCompleted& CompletedCallback) override;
	void AkAudioDevice_CreateIOHook(FAkAudioDevice* AkAudioDevice) override;
	void AkAudioDevice_LoadInitialData(FAkAudioDevice* AkAudioDevice) override;
	void AkAudioDevice_UnloadAllSoundData(FAkAudioDevice* AkAudioDevice) override;
	void AkAudioDevice_ReloadAllSoundData(FAkAudioDevice* AkAudioDevice) override;

	// AkAudioEvent
	void AkAudioEvent_Load(UAkAudioEvent* AkAudioEvent) override;

	// AkGameplayStatics
	void AkGameplayStatics_LoadBank(UAkAudioBank* AkAudioBank, const FString& BankName, FWaitEndBankAction* NewAction) override;
	void AkGameplayStatics_LoadBankAsync(UAkAudioBank* AkAudioBank, const FOnAkBankCallback& BankLoadedCallback) override;
	void AkGameplayStatics_LoadBankByName(const FString& BankName) override;
	void AkGameplayStatics_LoadBanks(const TArray<UAkAudioBank*>& SoundBanks, bool SynchronizeSoundBanks) override;
	void AkGameplayStatics_UnloadBank(UAkAudioBank* Bank, const FString& BankName, FWaitEndBankAction* NewAction) override;
	void AkGameplayStatics_UnloadBankAsync(UAkAudioBank* Bank, const FOnAkBankCallback& BankUnloadedCallback) override;
	void AkGameplayStatics_UnloadBankByName(const FString& BankName) override;

	// FAkSDKExternalSourceArray
	void FAkSDKExternalSourceArray_Ctor(FAkSDKExternalSourceArray* Instance, const TArray<FAkExternalSourceInfo>& BlueprintArray) override;

private:
	void LoadAllSoundData(FAkAudioDevice* AkAudioDevice);
	FDelegateHandle OnMediaFreedHandle;
};
