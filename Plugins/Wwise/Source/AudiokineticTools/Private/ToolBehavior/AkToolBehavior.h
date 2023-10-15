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

#include "Core/Public/Containers/Array.h"
#include "Core/Public/Templates/SharedPointer.h"

#include "AssetManagement/AkSoundDataBuilder.h"

class AkAssetDatabase;
class SWindow;
class UAkAudioBank;
class UAkAudioType;

struct PropertyToChange
{
	FString Name;
	FString Value;
	FString Xml;

	PropertyToChange(FString n, FString v, FString x)
		: Name(n)
		, Value(v)
		, Xml(x)
	{}
};

class AkToolBehavior
{
public:
	virtual ~AkToolBehavior() {}

	// CreateSoundDataWidget
	virtual bool CreateSoundDataWidget(const TSharedRef<SWindow>& Window, TArray<TWeakObjectPtr<UAkAudioBank>>* SoundBanks, bool ProjectSave) = 0;

	// AkAssetManagementManager
	virtual bool AkAssetManagementManager_ModifyProjectSettings(FString& ProjectContent);

	// AkAssetDatabase
	virtual FString AkAssetDatabase_GetInitBankPackagePath() const = 0;
	virtual bool AkAssetDatabase_ValidateAssetId(FGuid& outId) = 0;
	virtual bool AkAssetDatabase_Remove(AkAssetDatabase* Instance, const FAssetData& AssetData) = 0;

	// AkSoundDataBuilder
	virtual bool AkSoundDataBuilder_GetBankName(AkSoundDataBuilder* Instance, UAkAudioBank* Bank, const TSet<FString>& BanksToGenerate, FString& bankName) = 0;

	// CookAkSoundDataTask
	virtual TSharedPtr<AkSoundDataBuilder, ESPMode::ThreadSafe> CookAkSoundDataTask_CreateBuilder(const AkSoundDataBuilder::InitParameters& InitParameters) = 0;

	// WwiseConsoleAkSoundDataBuilder
	virtual FString WwiseConsoleAkSoundDataBuilder_AudioBankEventIncludes() const = 0;
	virtual FString WwiseConsoleAkSoundDataBuilder_AudioBankAuxBusIncludes() const = 0;

	// AkAssetFactory
	virtual bool AkAssetFactory_ValidNewAssetPath(FName Name, const FString& AssetPath, const UClass* AssetClass) const = 0;

	static AkToolBehavior* Get();
	static void ForceEventBasedToolBehavior();

protected:
	bool InsertProperties(const TArray<PropertyToChange>& PropertiesToChange, FString& ProjectContent);

private:
	static AkToolBehavior* s_Instance;
};