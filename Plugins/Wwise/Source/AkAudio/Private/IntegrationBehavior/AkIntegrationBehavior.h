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
#include "AkGameplayTypes.h"

class UAkAssetData;
class UAkAudioBank;
class UAkAudioEvent;
class UAkSettings;

class AkIntegrationBehavior
{
public:
	static AkIntegrationBehavior* Get();

	// AkAssetData
	virtual AKRESULT AkAssetData_LoadBank(UAkAssetData* AkAssetData) = 0;

	// AkAudioBank
	virtual void AkAudioBank_Load(UAkAudioBank* AkAudioBank) = 0;
	virtual void AkAudioBank_Unload(UAkAudioBank* AkAudioBank) = 0;

	// AkAudioDevice
	virtual AKRESULT AkAudioDevice_ClearBanks(FAkAudioDevice* AkAudioDevice) = 0;
	virtual AKRESULT AkAudioDevice_LoadInitBank(FAkAudioDevice* AkAudioDevice) = 0;
	virtual bool AkAudioDevice_LoadAllFilePackages(FAkAudioDevice* AkAudioDevice) = 0;
	virtual bool AkAudioDevice_UnloadAllFilePackages(FAkAudioDevice* AkAudioDevice) = 0;
	virtual void AkAudioDevice_LoadAllReferencedBanks(FAkAudioDevice* AkAudioDevice) = 0;
	virtual void AkAudioDevice_SetCurrentAudioCulture(const FString& NewWwiseLanguage) = 0;
	virtual void AkAudioDevice_SetCurrentAudioCultureAsync(FAkAudioDevice* AkAudioDevice, const FString& NewWwiseLanguage, const FOnSetCurrentAudioCultureCompleted& CompletedCallback) = 0;
	virtual void AkAudioDevice_SetCurrentAudioCultureAsync(FAkAudioDevice* AkAudioDevice, const FString& NewWwiseLanguage, FSetCurrentAudioCultureAction* LatentAction) = 0;
	virtual void AkAudioDevice_CreateIOHook(FAkAudioDevice* AkAudioDevice) = 0;
	virtual void AkAudioDevice_LoadInitialData(FAkAudioDevice* AkAudioDevice) = 0;
	virtual void AkAudioDevice_UnloadAllSoundData(FAkAudioDevice* AkAudioDevice) = 0;
	virtual void AkAudioDevice_ReloadAllSoundData(FAkAudioDevice* AkAudioDevice) = 0;

	// AkAudioEvent
	virtual void AkAudioEvent_Load(UAkAudioEvent* AkAudioEvent) = 0;

	// AkGameplayStatics
	virtual void AkGameplayStatics_LoadBank(UAkAudioBank* AkAudioBank, const FString& BankName, FWaitEndBankAction* NewAction) = 0;
	virtual void AkGameplayStatics_LoadBankAsync(UAkAudioBank* AkAudioBank, const FOnAkBankCallback& BankLoadedCallback) = 0;
	virtual void AkGameplayStatics_LoadBankByName(const FString& BankName) = 0;
	virtual void AkGameplayStatics_LoadBanks(const TArray<UAkAudioBank*>& SoundBanks, bool SynchronizeSoundBanks) = 0;
	virtual void AkGameplayStatics_UnloadBank(UAkAudioBank* Bank, const FString& BankName, FWaitEndBankAction* NewAction) = 0;
	virtual void AkGameplayStatics_UnloadBankAsync(UAkAudioBank* Bank, const FOnAkBankCallback& BankUnloadedCallback) = 0;
	virtual void AkGameplayStatics_UnloadBankByName(const FString& BankName) = 0;

	// FAkSDKExternalSourceArray
	virtual void FAkSDKExternalSourceArray_Ctor(FAkSDKExternalSourceArray* Instance, const TArray<FAkExternalSourceInfo>& BlueprintArray) = 0;

private:
	static AkIntegrationBehavior* s_Instance;
};
