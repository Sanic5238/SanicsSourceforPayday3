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

#include "AkToolBehavior.h"

class AkLegacyToolBehavior : public AkToolBehavior
{
public:
	using Super = AkToolBehavior;

	// CreateSoundDataWidget
	bool CreateSoundDataWidget(const TSharedRef<SWindow>& Window, TArray<TWeakObjectPtr<UAkAudioBank>>* SoundBanks, bool ProjectSave) override;

	// AkAssetManagementManager
	bool AkAssetManagementManager_ModifyProjectSettings(FString& ProjectContent) override;

	// AkAssetDatabase
	FString AkAssetDatabase_GetInitBankPackagePath() const override;
	bool AkAssetDatabase_ValidateAssetId(FGuid& outId) override;
	bool AkAssetDatabase_Remove(AkAssetDatabase* Instance, const FAssetData& AssetDAta) override;

	// AkSoundDataBuilder
	bool AkSoundDataBuilder_GetBankName(AkSoundDataBuilder* Instance, UAkAudioBank* Bank, const TSet<FString>& BanksToGenerate, FString& bankName) override;

	// CookAkSoundDataTask
	TSharedPtr<AkSoundDataBuilder, ESPMode::ThreadSafe> CookAkSoundDataTask_CreateBuilder(const AkSoundDataBuilder::InitParameters& InitParameters) override;

	// WwiseConsoleAkSoundDataBuilder
	FString WwiseConsoleAkSoundDataBuilder_AudioBankEventIncludes() const override;
	FString WwiseConsoleAkSoundDataBuilder_AudioBankAuxBusIncludes() const override;

	// AkAssetFactory
	bool AkAssetFactory_ValidNewAssetPath(FName Name, const FString& AssetPath, const UClass* AssetClass) const override;
};