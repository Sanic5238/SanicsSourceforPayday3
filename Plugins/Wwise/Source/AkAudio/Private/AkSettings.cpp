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


#include "AkSettings.h"

#include "AkAcousticTexture.h"
#include "AkAuxBus.h"
#include "AkAudioDevice.h"
#include "AkAudioEvent.h"
#include "AkSettingsPerUser.h"
#include "AkUnrealHelper.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "StringMatchAlgos/Array2D.h"
#include "StringMatchAlgos/StringMatching.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "AkAudioStyle.h"
#include "AkMediaAsset.h"
#include "AssetTools/Public/AssetToolsModule.h"
#if UE_5_0_OR_LATER
#include "HAL/PlatformFileManager.h"
#else
#include "HAL/PlatformFilemanager.h"
#endif
#include "InitializationSettings/AkInitializationSettings.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Platforms/AkUEPlatform.h"
#include "Settings/ProjectPackagingSettings.h"
#include "SettingsEditor/Public/ISettingsEditorModule.h"
#include "UnrealEd/Public/ObjectTools.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextbox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Views/SExpanderArrow.h"

#if AK_SUPPORT_WAAPI
#include "AkWaapiClient.h"
#include "AkWaapiUtils.h"
#include "Async/Async.h"

bool WAAPIGetTextureParams(FGuid textureID, FAkAcousticTextureParams& params)
{
	auto waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr)
	{
		/* Construct the relevant WAAPI json fields. */
		TArray<TSharedPtr<FJsonValue>> fromID;
		fromID.Add(MakeShareable(new FJsonValueString(textureID.ToString(EGuidFormats::DigitsWithHyphensInBraces))));

		TSharedRef<FJsonObject> getArgsJson = FAkWaapiClient::CreateWAAPIGetArgumentJson(FAkWaapiClient::WAAPIGetFromOption::ID, fromID);

		TSharedRef<FJsonObject> options = MakeShareable(new FJsonObject());
		TArray<TSharedPtr<FJsonValue>> StructJsonArray;
		StructJsonArray.Add(MakeShareable(new FJsonValueString("id")));
		TArray<FString> absorptionStrings{ "@AbsorptionLow", "@AbsorptionMidLow", "@AbsorptionMidHigh", "@AbsorptionHigh" };
		for (int i = 0; i < absorptionStrings.Num(); ++i)
			StructJsonArray.Add(MakeShareable(new FJsonValueString(absorptionStrings[i])));

		options->SetArrayField(FAkWaapiClient::WAAPIStrings::RETURN, StructJsonArray);

		TSharedPtr<FJsonObject> outJsonResult;
		if (waapiClient->Call(ak::wwise::core::object::get, getArgsJson, options, outJsonResult, 500, false))
		{
			/* Get absorption values from WAAPI return json. */
			TArray<TSharedPtr<FJsonValue>> returnJson = outJsonResult->GetArrayField(FAkWaapiClient::WAAPIStrings::RETURN);
			if (returnJson.Num() > 0)
			{
				auto jsonObj = returnJson[0]->AsObject();
				if (jsonObj != nullptr)
				{
					TSharedPtr<FJsonObject> absorptionObject = nullptr;
					for (int i = 0; i < absorptionStrings.Num(); ++i)
					{
						params.AbsorptionValues[i] = (float)(jsonObj->GetNumberField(absorptionStrings[i])) / 100.0f;
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool WAAPIGetObjectColorIndex(FGuid textureID, int& index)
{
	auto waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr)
	{
		/* Construct the relevant WAAPI json fields. */
		TArray<TSharedPtr<FJsonValue>> fromID;
		fromID.Add(MakeShareable(new FJsonValueString(textureID.ToString(EGuidFormats::DigitsWithHyphensInBraces))));
		TSharedRef<FJsonObject> getArgsJson = FAkWaapiClient::CreateWAAPIGetArgumentJson(FAkWaapiClient::WAAPIGetFromOption::ID, fromID);

		TSharedRef<FJsonObject> options = MakeShareable(new FJsonObject());
		TArray<TSharedPtr<FJsonValue>> StructJsonArray;
		StructJsonArray.Add(MakeShareable(new FJsonValueString("id")));
		StructJsonArray.Add(MakeShareable(new FJsonValueString("@Color")));

		options->SetArrayField(FAkWaapiClient::WAAPIStrings::RETURN, StructJsonArray);

		TSharedPtr<FJsonObject> outJsonResult;
		if (waapiClient->Call(ak::wwise::core::object::get, getArgsJson, options, outJsonResult, 500, false))
		{
			/* Get absorption values from WAAPI return json. */
			TArray<TSharedPtr<FJsonValue>> returnJson = outJsonResult->GetArrayField(FAkWaapiClient::WAAPIStrings::RETURN);
			if (returnJson.Num() > 0)
			{
				auto jsonObj = returnJson[0]->AsObject();
				if (jsonObj != nullptr)
				{
					index = (int)(jsonObj->GetNumberField("@Color"));
					return true;
				}
			}
		}
	}
	return false;
}

bool WAAPIGetObjectOverrideColor(FGuid textureID)
{
	auto waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr)
	{
		/* Construct the relevant WAAPI json fields. */
		TArray<TSharedPtr<FJsonValue>> fromID;
		fromID.Add(MakeShareable(new FJsonValueString(textureID.ToString(EGuidFormats::DigitsWithHyphensInBraces))));
		TSharedRef<FJsonObject> getArgsJson = FAkWaapiClient::CreateWAAPIGetArgumentJson(FAkWaapiClient::WAAPIGetFromOption::ID, fromID);

		TSharedRef<FJsonObject> options = MakeShareable(new FJsonObject());
		TArray<TSharedPtr<FJsonValue>> StructJsonArray;
		StructJsonArray.Add(MakeShareable(new FJsonValueString("id")));
		StructJsonArray.Add(MakeShareable(new FJsonValueString("@OverrideColor")));

		options->SetArrayField(FAkWaapiClient::WAAPIStrings::RETURN, StructJsonArray);

		TSharedPtr<FJsonObject> outJsonResult;
		if (waapiClient->Call(ak::wwise::core::object::get, getArgsJson, options, outJsonResult, 500, false))
		{
			/* Get absorption values from WAAPI return json. */
			TArray<TSharedPtr<FJsonValue>> returnJson = outJsonResult->GetArrayField(FAkWaapiClient::WAAPIStrings::RETURN);
			if (returnJson.Num() > 0)
			{
				auto jsonObj = returnJson[0]->AsObject();
				if (jsonObj != nullptr)
				{
					return jsonObj->GetBoolField("@OverrideColor");
				}
			}
		}
	}
	return false;
}
#endif // AK_SUPPORT_WAAPI
#endif // WITH_EDITOR

//////////////////////////////////////////////////////////////////////////
// UAkSettings

namespace AkSettings_Helper
{
#if WITH_EDITOR
	void MigrateMultiCoreRendering(bool EnableMultiCoreRendering, const FString& PlatformName)
	{
		FString SettingsClassName = FString::Format(TEXT("/Script/AkAudio.Ak{0}InitializationSettings"), { *PlatformName });
		auto* SettingsClass = AkUEFindObject(SettingsClassName);
		if (!SettingsClass)
		{
			return;
		}

		auto* MigrationFunction = SettingsClass->FindFunctionByName(TEXT("MigrateMultiCoreRendering"));
		auto* Settings = SettingsClass->GetDefaultObject();
		if (!MigrationFunction || !Settings)
		{
			return;
		}

		Settings->ProcessEvent(MigrationFunction, &EnableMultiCoreRendering);

		AkUnrealHelper::SaveConfigFile(Settings);
	}
#endif

	void MatchAcousticTextureNamesToPhysMaterialNames(
		const TArray<FAssetData>& PhysicalMaterials,
		const TArray<FAssetData>& AcousticTextures,
		TArray<int32>& assignments)
	{
		uint32 NumPhysMat = (uint32)PhysicalMaterials.Num();
		uint32 NumAcousticTex = (uint32)AcousticTextures.Num();

		// Create a scores matrix
		Array2D<float> scores(NumPhysMat, NumAcousticTex, 0);

		for (uint32 i = 0; i < NumPhysMat; ++i)
		{
			TArray<bool> perfectObjectMatches;
			perfectObjectMatches.Init(false, NumAcousticTex);

			if (PhysicalMaterials[i].GetAsset())
			{
				FString physMaterialName = PhysicalMaterials[i].GetAsset()->GetName();

				if (physMaterialName.Len() == 0)
					continue;

				for (uint32 j = 0; j < NumAcousticTex; ++j)
				{
					// Skip objects for which we already found a perfect match
					if (perfectObjectMatches[j] == true)
						continue;

					if (AcousticTextures[j].GetAsset())
					{
						FString acousticTextureName = AcousticTextures[j].GetAsset()->GetName();

						if (acousticTextureName.Len() == 0)
							continue;

						// Calculate longest common substring length
						float lcs = LCS::GetLCSScore(
							physMaterialName.ToLower(),
							acousticTextureName.ToLower());

						scores(i, j) = lcs;

						if (FMath::IsNearlyEqual(lcs, 1.f))
						{
							assignments[i] = j;
							perfectObjectMatches[j] = true;
							break;
						}
					}
				}
			}
		}

		for (uint32 i = 0; i < NumPhysMat; ++i)
		{
			if (assignments[i] == -1)
			{
				float bestScore = 0.f;
				int32 matchedIdx = -1;
				for (uint32 j = 0; j < NumAcousticTex; ++j)
				{
					if (scores(i, j) > bestScore)
					{
						bestScore = scores(i, j);
						matchedIdx = j;
					}
				}
				if (bestScore >= 0.2f)
					assignments[i] = matchedIdx;
			}
		}
	}
}

FString UAkSettings::DefaultSoundDataFolder = TEXT("WwiseAudio");

#if WITH_EDITOR
float UAkSettings::MinimumDecayKeyDistance = 0.01f;
#endif

UAkSettings::UAkSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WwiseSoundDataFolder.Path = DefaultSoundDataFolder;

	GlobalDecayAbsorption = 0.5f;

#if WITH_EDITOR
	AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	//register to asset modification delegates
	auto& AssetRegistry = AssetRegistryModule->Get();
	AssetRegistry.OnAssetAdded().AddUObject(this, &UAkSettings::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddUObject(this, &UAkSettings::OnAssetRemoved);
	VisualizeRoomsAndPortals = false;
#endif // WITH_EDITOR
}

UAkSettings::~UAkSettings()
{
#if WITH_EDITOR
#if AK_SUPPORT_WAAPI
	FAkWaapiClient* waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr)
	{
		if (WaapiProjectLoadedHandle.IsValid())
		{
			waapiClient->OnProjectLoaded.Remove(WaapiProjectLoadedHandle);
			WaapiProjectLoadedHandle.Reset();
		}
		if (WaapiConnectionLostHandle.IsValid())
		{
			waapiClient->OnConnectionLost.Remove(WaapiConnectionLostHandle);
			WaapiConnectionLostHandle.Reset();
		}
		ClearWaapiTextureCallbacks();
	}
#endif
#endif
}

void UAkSettings::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	UAkSettingsPerUser* AkSettingsPerUser = GetMutableDefault<UAkSettingsPerUser>();

	if (AkSettingsPerUser)
	{
		bool didChanges = false;

		if (!WwiseWindowsInstallationPath_DEPRECATED.Path.IsEmpty())
		{
			AkSettingsPerUser->WwiseWindowsInstallationPath = WwiseWindowsInstallationPath_DEPRECATED;
			WwiseWindowsInstallationPath_DEPRECATED.Path.Reset();
			didChanges = true;
		}

		if (!WwiseMacInstallationPath_DEPRECATED.FilePath.IsEmpty())
		{
			AkSettingsPerUser->WwiseMacInstallationPath = WwiseMacInstallationPath_DEPRECATED;
			WwiseMacInstallationPath_DEPRECATED.FilePath.Reset();
			didChanges = true;
		}

		if (bAutoConnectToWAAPI_DEPRECATED)
		{
			AkSettingsPerUser->bAutoConnectToWAAPI = true;
			bAutoConnectToWAAPI_DEPRECATED = false;
			didChanges = true;
		}

		if (EnableAutomaticAssetSynchronization_DEPRECATED)
		{
			AkSettingsPerUser->EnableAutomaticAssetSynchronization = true;
			EnableAutomaticAssetSynchronization_DEPRECATED = false;
			didChanges = true;
		}

		if (didChanges)
		{
			AkUnrealHelper::SaveConfigFile(this);
			AkSettingsPerUser->SaveConfig();
		}
	}

	if (!MigratedEnableMultiCoreRendering)
	{
		MigratedEnableMultiCoreRendering = true;

		for (const auto& PlatformName : AkUnrealPlatformHelper::GetAllSupportedWwisePlatforms())
		{
			AkSettings_Helper::MigrateMultiCoreRendering(bEnableMultiCoreRendering_DEPRECATED, *PlatformName);
		}
	}

	PreviousDecayAuxBusMap = EnvironmentDecayAuxBusMap;
#endif // WITH_EDITOR
}

#if WITH_EDITOR
#if UE_4_25_OR_LATER
void UAkSettings::PreEditChange(FProperty* PropertyAboutToChange)
#else
void UAkSettings::PreEditChange(UProperty* PropertyAboutToChange)
#endif
{
	PreviousWwiseProjectPath = WwiseProjectPath.FilePath;
	PreviousWwiseSoundBankFolder = WwiseSoundDataFolder.Path;
}

#define LOCTEXT_NAMESPACE "AkAudio"
void UAkSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	ISettingsEditorModule& SettingsEditorModule = FModuleManager::GetModuleChecked<ISettingsEditorModule>("SettingsEditor");

	if ( PropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, MaxSimultaneousReverbVolumes) )
	{
		MaxSimultaneousReverbVolumes = FMath::Clamp<uint8>( MaxSimultaneousReverbVolumes, 0, AK_MAX_AUX_PER_OBJ );
		FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
		if( AkAudioDevice )
		{
			AkAudioDevice->SetMaxAuxBus(MaxSimultaneousReverbVolumes);
		}
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, WwiseProjectPath))
	{
		AkUnrealHelper::SanitizeProjectPath(WwiseProjectPath.FilePath, PreviousWwiseProjectPath, FText::FromString("Please enter a valid Wwise project"), bRequestRefresh);
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, WwiseSoundDataFolder))
	{
		if (UseEventBasedPackaging)
		{
			FString oldSoundDataPackagePath = FString::Printf(TEXT("/Game/%s"), *PreviousWwiseSoundBankFolder);
			FString newSoundDataPackagePath = FString::Printf(TEXT("/Game/%s"), *WwiseSoundDataFolder.Path);

			OnSoundDataFolderChanged.Broadcast(oldSoundDataPackagePath, newSoundDataPackagePath);

			UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();

			FDirectoryPath newPath;
			newPath.Path = newSoundDataPackagePath;
			PackagingSettings->DirectoriesToAlwaysCook.Add(newPath);

			RemoveSoundDataFromAlwaysCook(oldSoundDataPackagePath);
			AkUnrealHelper::SaveConfigFile(PackagingSettings);
		}
		else
		{
			for(auto& platform : AkUnrealPlatformHelper::GetAllSupportedWwisePlatforms(true))
			{
				AkUnrealHelper::UpdateGeneratedSoundBanksFolder(*platform, PreviousWwiseSoundBankFolder);
			}
		}

		SettingsEditorModule.OnApplicationRestartRequired();
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, UseEventBasedPackaging))
	{
		bool RestartRequired = true;
		if (UseEventBasedPackaging)
		{
			auto YesDelegate = FOnButtonClickedMigration::CreateLambda([this]() -> FReply {
				EnsureSoundDataPathIsInAlwaysCook();
				RemoveSoundDataFromAlwaysStageAsUFS(WwiseSoundDataFolder.Path);
				AkUnrealHelper::DeleteOldSoundBanks();
				OnActivatedNewAssetManagement.Broadcast();
				return FReply::Handled();
			});

			auto NoDelegate = FOnButtonClickedMigration::CreateLambda([this, &RestartRequired]() -> FReply {
				UseEventBasedPackaging = false;
				RestartRequired = false;
				return FReply::Handled();
			});

			AkUnrealHelper::ShowEventBasedPackagingMigrationDialog(YesDelegate, NoDelegate);
		}
		else
		{
			TArray<FAssetData> AssetsFound;
			FString AssetPath = FString(TEXT("/Game")) / WwiseSoundDataFolder.Path;
			AssetRegistryModule->Get().GetAssetsByPath(FName(*AssetPath), AssetsFound, true);
			if (AssetsFound.Num() > 0)
			{
				FText NewSoundDataFolder;

				TSharedPtr<SWindow> Dialog = SNew(SWindow)
					.Title(LOCTEXT("DisableEBPTitle", "Disable Event-Based Packaging"))
					.SupportsMaximize(false)
					.SupportsMinimize(false)
					.FocusWhenFirstShown(true)
					.SizingRule(ESizingRule::Autosized);

				TSharedRef<SWidget> DialogContent = 
					SNew(SBorder)
					.BorderImage(FAkAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.FillHeight(0.45f)
						[
							SNew(SSpacer)
						]
						+ SVerticalBox::Slot()
						.HAlign(EHorizontalAlignment::HAlign_Left)
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AssetsInSoundData", "Assets found in the Sound Data folder. Please choose a new destination folder for your SoundBanks."))
						]
						+ SVerticalBox::Slot()
						.FillHeight(0.1f)
						[
							SNew(SSpacer)
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2, 2, 6, 2)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("SoundDataFolder", "Sound Data Folder:"))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2, 0, 2, 0)
							[
								SNew(SEditableTextBox)
								.HintText(LOCTEXT("SoundDataFolderHint", "Name of the Sound Data Folder"))
								.OnTextCommitted_Lambda([&NewSoundDataFolder, &Dialog](const FText& InText, ETextCommit::Type InCommitType) {
									if (!InText.IsEmpty())
									{
										NewSoundDataFolder = InText;
										Dialog->RequestDestroyWindow();
									}
								})
								.MinDesiredWidth(200)
								.RevertTextOnEscape(true)
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2, 0, 2, 0)
							[
								SNew(SButton)
								.OnClicked_Lambda([Dialog]() -> FReply { Dialog->RequestDestroyWindow(); return FReply::Handled(); })
								.Text(LOCTEXT("OkButtonLabel", "OK"))
							]
						]
						+ SVerticalBox::Slot()
						.FillHeight(0.45f)
						[
							SNew(SSpacer)
						]
					];

				Dialog->SetContent(DialogContent);
				FSlateApplication::Get().AddModalWindow(Dialog.ToSharedRef(), nullptr);
				WwiseSoundDataFolder.Path = NewSoundDataFolder.ToString();
				AkUnrealHelper::SaveConfigFile(this);
			}

			AddSoundDataToAlwaysStageAsUFS();
			RemoveSoundDataFromAlwaysCook(FString::Printf(TEXT("/Game/%s"), *PreviousWwiseSoundBankFolder));
		}

		if (RestartRequired)
		{
			SettingsEditorModule.OnApplicationRestartRequired();
		}
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, AkGeometryMap))
	{
		if (PropertyName == GET_MEMBER_NAME_CHECKED(FAkGeometrySurfacePropertiesToMap, AcousticTexture))
		{
			for (auto& elem : AkGeometryMap)
			{
				PhysicalMaterialAcousticTextureMap[elem.Key.LoadSynchronous()] = elem.Value.AcousticTexture.LoadSynchronous();
			}
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FAkGeometrySurfacePropertiesToMap, OcclusionValue))
		{
			for (auto& elem : AkGeometryMap)
			{
				PhysicalMaterialOcclusionMap[elem.Key.LoadSynchronous()] = elem.Value.OcclusionValue;
			}
		}
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, VisualizeRoomsAndPortals))
	{
		OnShowRoomsPortalsChanged.Broadcast();
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, EnvironmentDecayAuxBusMap))
	{
		if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
		{
			DecayAuxBusMapChanged();
			OnAuxBusAssignmentMapChanged.Broadcast();
		}
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, GlobalDecayAbsorption))
	{
		OnAuxBusAssignmentMapChanged.Broadcast();
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, DefaultReverbAuxBus))
	{
		OnAuxBusAssignmentMapChanged.Broadcast();
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, HFDampingName)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, DecayEstimateName)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, TimeToFirstReflectionName)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, HFDampingRTPC)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, DecayEstimateRTPC)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, TimeToFirstReflectionRTPC))
	{
		OnReverbRTPCChanged.Broadcast();
	}
	else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAkSettings, SplitMediaPerFolder))
	{
		SplitOrMergeMedia();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#undef LOCTEXT_NAMESPACE

void UAkSettings::FillAkGeometryMap(
	const TArray<FAssetData>& PhysicalMaterialAssets,
	const TArray<FAssetData>& AcousticTextureAssets)
{
	TArray<int32> assignments;
	assignments.Init(-1, PhysicalMaterialAssets.Num());

	AkSettings_Helper::MatchAcousticTextureNamesToPhysMaterialNames(PhysicalMaterialAssets, AcousticTextureAssets, assignments);

	for (int i = 0; i < PhysicalMaterialAssets.Num(); i++)
	{
		auto physicalMaterial = Cast<UPhysicalMaterial>(PhysicalMaterialAssets[i].GetAsset());
		if (!PhysicalMaterialAcousticTextureMap.Contains(physicalMaterial))
		{
			if (assignments[i] != -1)
			{
				int32 acousticTextureIdx = assignments[i];
				auto acousticTexture = Cast<UAkAcousticTexture>(AcousticTextureAssets[acousticTextureIdx].GetAsset());
				PhysicalMaterialAcousticTextureMap.Add(physicalMaterial, acousticTexture);
			}
			else
			{
				PhysicalMaterialAcousticTextureMap.Add(physicalMaterial);
			}
		}
		else
		{
			if (assignments[i] != -1)
			{
				if (!PhysicalMaterialAcousticTextureMap[physicalMaterial])
				{
					int32 acousticTextureIdx = assignments[i];
					auto acousticTexture = Cast<UAkAcousticTexture>(AcousticTextureAssets[acousticTextureIdx].GetAsset());
					PhysicalMaterialAcousticTextureMap[physicalMaterial] = acousticTexture;
				}
			}
		}

		if (!PhysicalMaterialOcclusionMap.Contains(physicalMaterial))
			PhysicalMaterialOcclusionMap.Add(physicalMaterial, 1.f);
	}

	UpdateAkGeometryMap();
}

void UAkSettings::UpdateAkGeometryMap()
{
	decltype(AkGeometryMap) UpdatedGeometryMap;
	for (const auto& AcousticTextureTuple : PhysicalMaterialAcousticTextureMap)
	{
		const auto* PhysicalMaterial(AcousticTextureTuple.Key);
		const auto* AcousticTexture(AcousticTextureTuple.Value);
		const auto* OcclusionPtr = PhysicalMaterialOcclusionMap.Find(PhysicalMaterial);
		if (UNLIKELY(!OcclusionPtr))
		{
			UE_LOG(LogAkAudio, Warning, TEXT("UpdateAkGeometryMap: Could not find Occlusion of Physical Material %s with Acoustic Texture %s"),
				PhysicalMaterial ? *PhysicalMaterial->GetFName().ToString() : TEXT("[nullptr]"),
				AcousticTexture ? *AcousticTexture->GetFName().ToString() : TEXT("[nullptr]"));
			continue;
		}

		FAkGeometrySurfacePropertiesToMap SurfaceProperties;
		SurfaceProperties.AcousticTexture = AcousticTexture;
		SurfaceProperties.OcclusionValue = *OcclusionPtr;
		UpdatedGeometryMap.Emplace(PhysicalMaterial, MoveTemp(SurfaceProperties));
	}

	UpdatedGeometryMap.KeySort([](const auto& Lhs, const auto& Rhs) {
		if (UNLIKELY(!Lhs.IsValid() || !Rhs.IsValid()))
		{
			return !Lhs.IsValid();
		}
		return Lhs->GetPathName().Compare(Rhs->GetPathName()) < 0;
	});

	if (!UpdatedGeometryMap.OrderIndependentCompareEqual(AkGeometryMap))
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("UpdateAkGeometryMap: Updating changed AkGeometryMap"));
		AkGeometryMap = UpdatedGeometryMap;
		AkUnrealHelper::SaveConfigFile(this);
	}
	else
	{
		UE_LOG(LogAkAudio, VeryVerbose, TEXT("UpdateAkGeometryMap: AkGeometryMap is unchanged. Skip updating."));
	}
}

void UAkSettings::InitAkGeometryMap()
{
	PhysicalMaterialAcousticTextureMap.Empty();
	PhysicalMaterialOcclusionMap.Empty();

	// copy everything from the ini file
	for (auto& elem : AkGeometryMap)
	{
		auto physMat = elem.Key.LoadSynchronous();
		auto surfaceProps = elem.Value;
		PhysicalMaterialAcousticTextureMap.Add(physMat, surfaceProps.AcousticTexture.LoadSynchronous());
		PhysicalMaterialOcclusionMap.Add(physMat, surfaceProps.OcclusionValue);
	}
	bAkGeometryMapInitialized = true;

	// Obtain the 2 list of children we want to match
	TArray<FAssetData> PhysicalMaterials, AcousticTextures;
	AkUEGetAssetsByClass(UPhysicalMaterial::StaticClass(), PhysicalMaterials);
	AkUEGetAssetsByClass(UAkAcousticTexture::StaticClass(), AcousticTextures);

	FillAkGeometryMap(PhysicalMaterials, AcousticTextures);
}

void UAkSettings::ClearAkRoomDecayAuxBusMap()
{
	EnvironmentDecayAuxBusMap.Empty();
	PreviousDecayAuxBusMap = EnvironmentDecayAuxBusMap;
}

void UAkSettings::InsertDecayKeyValue(const float& decayKey)
{
	if (decayKey < 0.0f)
	{
		UE_LOG(LogAkAudio, Warning, TEXT("AkSettings: Reverb Assignment Map: Decay key values must be positive."));
		return;
	}
	// Refuse key value if it is too close to an existing key value (within MinimumDecayKeyDistance).
	TArray<float> decayKeys;
	EnvironmentDecayAuxBusMap.GetKeys(decayKeys);
	for (int i = 0; i < decayKeys.Num(); ++i)
	{
		if (FMath::Abs(decayKeys[i] - decayKey) < MinimumDecayKeyDistance)
		{
			UE_LOG(LogAkAudio, Warning, TEXT("AkSettings: Reverb Assignment Map: New decay key too close to existing key. Must be +- %f from any existing key."), MinimumDecayKeyDistance);
			return;
		}
		// Keys are reverse sorted. If the new key value is larger enough than the current index, we have found a valid insert space.
		// (There will be no other keys larger than the current index to check against).
		if (decayKey - decayKeys[i] > MinimumDecayKeyDistance)
			break;
	}
	EnvironmentDecayAuxBusMap.Add(decayKey, nullptr);
	SortDecayKeys();
}

void UAkSettings::SetAcousticTextureParams(const FGuid& textureID, const FAkAcousticTextureParams& params)
{
	if (AcousticTextureParamsMap.Contains(textureID))
		AcousticTextureParamsMap[textureID] = params;
	else
		AcousticTextureParamsMap.Add(textureID, params);

#if AK_SUPPORT_WAAPI
	RegisterWaapiTextureCallback(textureID);
#endif
}

void UAkSettings::ClearTextureParamsMap()
{
	AcousticTextureParamsMap.Empty();

#if AK_SUPPORT_WAAPI
	ClearWaapiTextureCallbacks();
#endif
}

#if AK_SUPPORT_WAAPI
void UAkSettings::WaapiProjectLoaded()
{
	TArray<FGuid> keys;
	AcousticTextureParamsMap.GetKeys(keys);
	for (auto key : keys)
	{
		UpdateTextureParams(key);
		UpdateTextureColor(key);
		RegisterWaapiTextureCallback(key);
	}
}

void UAkSettings::WaapiDisconnected()
{
	ClearWaapiTextureCallbacks();
}

void UAkSettings::RegisterWaapiTextureCallback(const FGuid& textureID)
{
	FAkWaapiClient* waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr && waapiClient->IsConnected())
	{
		auto absorptionCallback = WampEventCallback::CreateLambda([this](uint64_t id, TSharedPtr<FJsonObject> jsonObject)
		{
			const TSharedPtr<FJsonObject> itemObj = jsonObject->GetObjectField(WwiseWaapiHelper::OBJECT);
			if (itemObj != nullptr)
			{
				const FString itemIdString = itemObj->GetStringField(WwiseWaapiHelper::ID);
				FGuid itemID = FGuid::NewGuid();
				FGuid::ParseExact(itemIdString, EGuidFormats::DigitsWithHyphensInBraces, itemID);
				if (AcousticTextureParamsMap.Find(itemID) != nullptr)
				{
					AsyncTask(ENamedThreads::GameThread, [this, itemID]
					{
						UpdateTextureParams(itemID);
					});
				}
			}
		});


		TSharedRef<FJsonObject> options = MakeShareable(new FJsonObject());
		options->SetStringField(WwiseWaapiHelper::OBJECT, textureID.ToString(EGuidFormats::DigitsWithHyphensInBraces));

		TArray<FString> absorptionStrings{ "AbsorptionLow", "AbsorptionMidLow", "AbsorptionMidHigh", "AbsorptionHigh" };
		TSharedPtr<FJsonObject> jsonResult;
		TSharedPtr<FJsonObject> unsubscribeResult;
		bool unsubscribeNeeded = WaapiTextureSubscriptions.Find(textureID) != nullptr;
		TArray<uint64> subscriptionIDs{ 0,0,0,0 };
		for (int i = 0; i < absorptionStrings.Num(); ++i)
		{
			options->SetStringField(WwiseWaapiHelper::PROPERTY, absorptionStrings[i]);
			if (unsubscribeNeeded)
			{
				waapiClient->Unsubscribe(WaapiTextureSubscriptions[textureID][i], unsubscribeResult);
			}
			if (!waapiClient->Subscribe(ak::wwise::core::object::propertyChanged, options, absorptionCallback, subscriptionIDs[i], jsonResult))
			{
				UE_LOG(LogAkAudio, Warning, TEXT("AkSettings: WAAPI: Acoustic texture propertyChanged subscription failed."));
			}
		}
		WaapiTextureSubscriptions.Add(textureID, subscriptionIDs);


		auto colorCallback = WampEventCallback::CreateLambda([this](uint64_t id, TSharedPtr<FJsonObject> jsonObject)
		{
			const TSharedPtr<FJsonObject> itemObj = jsonObject->GetObjectField(WwiseWaapiHelper::OBJECT);
			if (itemObj != nullptr)
			{
				const FString itemIdString = itemObj->GetStringField(WwiseWaapiHelper::ID);
				FGuid itemID = FGuid::NewGuid();
				FGuid::ParseExact(itemIdString, EGuidFormats::DigitsWithHyphensInBraces, itemID);
				if (AcousticTextureParamsMap.Find(itemID) != nullptr)
				{
					AsyncTask(ENamedThreads::GameThread, [this, itemID]
					{
						UpdateTextureColor(itemID);
					});
				}
			}
		});

		options = MakeShareable(new FJsonObject());
		options->SetStringField(WwiseWaapiHelper::OBJECT, textureID.ToString(EGuidFormats::DigitsWithHyphensInBraces));
		unsubscribeNeeded = WaapiTextureColorSubscriptions.Find(textureID) != nullptr;
		uint64 subscriptionID = 0;
		options->SetStringField(WwiseWaapiHelper::PROPERTY, "Color");
		if (unsubscribeNeeded)
		{
			waapiClient->Unsubscribe(WaapiTextureColorSubscriptions[textureID], unsubscribeResult);
		}
		if (!waapiClient->Subscribe(ak::wwise::core::object::propertyChanged, options, colorCallback, subscriptionID, jsonResult))
		{
			UE_LOG(LogAkAudio, Warning, TEXT("AkSettings: WAAPI: Acoustic texture Color propertyChanged subscription failed."));
		}
		WaapiTextureColorSubscriptions.Add(textureID, subscriptionID);

		unsubscribeNeeded = WaapiTextureColorOverrideSubscriptions.Find(textureID) != nullptr;
		subscriptionID = 0;
		options->SetStringField(WwiseWaapiHelper::PROPERTY, "OverrideColor");
		if (unsubscribeNeeded)
		{
			waapiClient->Unsubscribe(WaapiTextureColorOverrideSubscriptions[textureID], unsubscribeResult);
		}
		if (!waapiClient->Subscribe(ak::wwise::core::object::propertyChanged, options, colorCallback, subscriptionID, jsonResult))
		{
			UE_LOG(LogAkAudio, Warning, TEXT("AkSettings: WAAPI: Acoustic texture OverrideColor propertyChanged subscription failed."));
		}
		WaapiTextureColorOverrideSubscriptions.Add(textureID, subscriptionID);
	}
}

void UAkSettings::UnregisterWaapiTextureCallback(const FGuid& textureID)
{
	FAkWaapiClient* waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr && waapiClient->IsConnected())
	{
		if (WaapiTextureSubscriptions.Find(textureID) != nullptr)
		{
			TSharedPtr<FJsonObject> unsubscribeResult;
			for (int i = 0; i < WaapiTextureSubscriptions[textureID].Num(); ++i)
				waapiClient->Unsubscribe(WaapiTextureSubscriptions[textureID][i], unsubscribeResult);
			WaapiTextureSubscriptions.Remove(textureID);
		}
		if (WaapiTextureColorSubscriptions.Find(textureID) != nullptr)
		{
			TSharedPtr<FJsonObject> unsubscribeResult;
			waapiClient->Unsubscribe(WaapiTextureColorSubscriptions[textureID], unsubscribeResult);
			WaapiTextureColorSubscriptions.Remove(textureID);
		}
		if (WaapiTextureColorOverrideSubscriptions.Find(textureID) != nullptr)
		{
			TSharedPtr<FJsonObject> unsubscribeResult;
			waapiClient->Unsubscribe(WaapiTextureColorOverrideSubscriptions[textureID], unsubscribeResult);
			WaapiTextureColorOverrideSubscriptions.Remove(textureID);
		}
	}
}

void UAkSettings::ClearWaapiTextureCallbacks()
{
	FAkWaapiClient* waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr && waapiClient->IsConnected())
	{
		for (auto it = WaapiTextureSubscriptions.CreateIterator(); it; ++it)
		{
			TSharedPtr<FJsonObject> unsubscribeResult;
			for (int i = 0; i < it.Value().Num(); ++i)
				waapiClient->Unsubscribe(it.Value()[i], unsubscribeResult);
		}
		for (auto it = WaapiTextureColorSubscriptions.CreateIterator(); it; ++it)
		{
			TSharedPtr<FJsonObject> unsubscribeResult;
			waapiClient->Unsubscribe(it.Value(), unsubscribeResult);
		}
		for (auto it = WaapiTextureColorOverrideSubscriptions.CreateIterator(); it; ++it)
		{
			TSharedPtr<FJsonObject> unsubscribeResult;
			waapiClient->Unsubscribe(it.Value(), unsubscribeResult);
		}
		WaapiTextureSubscriptions.Empty();
		WaapiTextureColorSubscriptions.Empty();
		WaapiTextureColorOverrideSubscriptions.Empty();
	}
}

void UAkSettings::UpdateTextureParams(const FGuid& textureID)
{
	WAAPIGetTextureParams(textureID, AcousticTextureParamsMap[textureID]);
	OnTextureParamsChanged.Broadcast(textureID);
}

void UAkSettings::UpdateTextureColor(const FGuid& textureID)
{
	if (!WAAPIGetObjectOverrideColor(textureID))
	{
		SetTextureColor(textureID, -1);
		return;
	}

	int colorIndex = 0;
	if (WAAPIGetObjectColorIndex(textureID, colorIndex))
	{
		SetTextureColor(textureID, colorIndex);
	}
}

void UAkSettings::SetTextureColor(FGuid textureID, int colorIndex)
{
	TArray<FAssetData> AcousticTextures;
	AkUEGetAssetsByClass(UAkAcousticTexture::StaticClass(), AcousticTextures);

	FLinearColor color = FAkAudioStyle::GetWwiseObjectColor(colorIndex);
	for (FAssetData& textureAsset : AcousticTextures)
	{
		if (UAkAcousticTexture* texture = Cast<UAkAcousticTexture>(textureAsset.GetAsset()))
		{
			if (texture->ID == textureID && texture->EditColor != color)
			{
				texture->Modify();
				texture->EditColor = color;
				break;
			}
		}
	}
}

#endif // AK_SUPPORT_WAAPI

void UAkSettings::DecayAuxBusMapChanged()
{
	// If a key has been moved beyond its neighbours, restrict it between neighbouring key values

	// Removal - nothing to restrict
	if (PreviousDecayAuxBusMap.Num() > EnvironmentDecayAuxBusMap.Num())
	{
		PreviousDecayAuxBusMap = EnvironmentDecayAuxBusMap;
		return;
	}
	// Addition - when the insert button is used, restrictions will already be handled.
	// If the stock Unreal '+' button is used, a 0-nullptr entry will be added at the end of the map. In this case,
	// remove it and insert it using the InsertDecayKeyValue so it can be properly restricted and placed.
	if (PreviousDecayAuxBusMap.Num() < EnvironmentDecayAuxBusMap.Num())
	{
		if (EnvironmentDecayAuxBusMap.Num() > 1)
		{
			TArray<float> keys;
			EnvironmentDecayAuxBusMap.GetKeys(keys);
			if (keys.Last() == 0.0f && EnvironmentDecayAuxBusMap[0.0f] == nullptr)
			{
				EnvironmentDecayAuxBusMap.Remove(0.0f);
				InsertDecayKeyValue(0.0f);
			}
		}
		PreviousDecayAuxBusMap = EnvironmentDecayAuxBusMap;
		return;
	}

	// Find key that has changed
	int changedKeyIndex = -1;
	const int numKeys = PreviousDecayAuxBusMap.Num();
	TArray<float> previousKeys;
	TArray<float> newKeys;
	PreviousDecayAuxBusMap.GetKeys(previousKeys);
	EnvironmentDecayAuxBusMap.GetKeys(newKeys);
	for (int i = 0; i < numKeys; ++i)
	{
		if (!FMath::IsNearlyEqual(previousKeys[i], newKeys[i], 1.0e-06F)) // Floating point property values have a tendancy to gradually wander in UE.
		{
			changedKeyIndex = i;
			break;
		}
	}
	// If no key values have changed, an aux bus has been changed. Nothing to restrict.
	if (changedKeyIndex == -1)
		return;
	// check key value
	float newKeyValue = newKeys[changedKeyIndex];
	float restrictedKeyValue = newKeyValue;
	FString restrictionInfoString;
	// Keys are sorted in reverse order such that they are ordered vertically from smallest to largest, in the UI.
	// So, check newKeyValue <= newKeys[changedKeyIndex + 1] and newKeyValue >= newKeys[changedKeyIndex - 1].
	const bool changedKeyIsSmallest = changedKeyIndex == numKeys - 1;
	float lowerLimit = changedKeyIsSmallest ? 0.0f : newKeys[changedKeyIndex + 1];
	if (newKeyValue <= lowerLimit)
	{
		restrictedKeyValue = changedKeyIsSmallest ? 0.0f : lowerLimit + MinimumDecayKeyDistance;
		restrictionInfoString = FString("Decay key value limited by next lowest value.");
	}
	else if (changedKeyIndex > 0 && newKeyValue >= newKeys[changedKeyIndex - 1])
	{
		restrictedKeyValue = newKeys[changedKeyIndex - 1] - MinimumDecayKeyDistance;
		restrictionInfoString = FString("Decay key value limited by next highest value.");
	}
	// If key value needs to be restricted, remove and replace the entry in the map.
	if (restrictedKeyValue != newKeyValue)
	{
		FAkAudioStyle::DisplayEditorMessgae(FText::FromString(restrictionInfoString));
		TSoftObjectPtr<UAkAuxBus> auxBusToMove = EnvironmentDecayAuxBusMap[newKeyValue];
		EnvironmentDecayAuxBusMap.Remove(newKeyValue);
		EnvironmentDecayAuxBusMap.Add(restrictedKeyValue, auxBusToMove);
		SortDecayKeys();
	}
	else // No restriction to apply, but still keep track of the new key values. 
	{
		PreviousDecayAuxBusMap = EnvironmentDecayAuxBusMap;
	}
}

void UAkSettings::SortDecayKeys()
{
	// high to low decay ('large' to 'small' environment structure)
	EnvironmentDecayAuxBusMap.KeySort([](const float& left, const float& right) {return left > right; });
	PreviousDecayAuxBusMap = EnvironmentDecayAuxBusMap;
}

void UAkSettings::OnAssetAdded(const FAssetData& NewAssetData)
{
	if (!bAkGeometryMapInitialized)
		return;

	if (DoesAssetMatchClass(NewAssetData, UPhysicalMaterial::StaticClass()))
	{
		if (auto physicalMaterial = Cast<UPhysicalMaterial>(NewAssetData.GetAsset()))
		{
			TArray<FAssetData> PhysicalMaterials, AcousticTextures;
			PhysicalMaterials.Add(NewAssetData);
			AkUEGetAssetsByClass(UAkAcousticTexture::StaticClass(), AcousticTextures);

			FillAkGeometryMap(PhysicalMaterials, AcousticTextures);
		}
	} 
	else if (DoesAssetMatchClass(NewAssetData, UAkAcousticTexture::StaticClass()))
	{
		if (auto acousticTexture = Cast<UAkAcousticTexture>(NewAssetData.GetAsset()))
		{
			acousticTexture->ValidateShortId(true);

			TArray<FAssetData> PhysicalMaterials, AcousticTextures;
			AkUEGetAssetsByClass(UPhysicalMaterial::StaticClass(), PhysicalMaterials);
			AcousticTextures.Add(NewAssetData);

			FillAkGeometryMap(PhysicalMaterials, AcousticTextures);
#if AK_SUPPORT_WAAPI
			FAkAcousticTextureParams params;
			bool paramsExist = AcousticTextureParamsMap.Contains(acousticTexture->ID);
			if (paramsExist)
				params = *AcousticTextureParamsMap.Find(acousticTexture->ID);
			bool paramsSet = WAAPIGetTextureParams(acousticTexture->ID, params);
			if (paramsSet && !paramsExist)
				AcousticTextureParamsMap.Add(acousticTexture->ID, params);
			RegisterWaapiTextureCallback(acousticTexture->ID);
			int colorIndex = -1;
			if (WAAPIGetObjectColorIndex(acousticTexture->ID, colorIndex))
			{
				acousticTexture->EditColor = FAkAudioStyle::GetWwiseObjectColor(colorIndex);
			}
#endif
		}
	}
}

void UAkSettings::OnAssetRemoved(const struct FAssetData& AssetData)
{
	if (DoesAssetMatchClass(AssetData, UPhysicalMaterial::StaticClass()))
	{
		if (auto physicalMaterial = Cast<UPhysicalMaterial>(AssetData.GetAsset()))
		{
			PhysicalMaterialAcousticTextureMap.Remove(physicalMaterial);
			PhysicalMaterialOcclusionMap.Remove(physicalMaterial);
			UpdateAkGeometryMap();
		}
	}
	else if(DoesAssetMatchClass(AssetData, UAkAcousticTexture::StaticClass()))
	{
		if(auto acousticTexture = Cast<UAkAcousticTexture>(AssetData.GetAsset()))
		{
			AcousticTextureParamsMap.Remove(acousticTexture->ID);
#if AK_SUPPORT_WAAPI
			UnregisterWaapiTextureCallback(acousticTexture->ID);
#endif
		}
	}
}

#if AK_SUPPORT_WAAPI
void UAkSettings::InitWaapiSync()
{
	FAkWaapiClient* waapiClient = FAkWaapiClient::Get();
	if (waapiClient != nullptr)
	{
		if (waapiClient->IsProjectLoaded())
			WaapiProjectLoaded();
		WaapiProjectLoadedHandle = waapiClient->OnProjectLoaded.AddLambda([this]()
			{
				WaapiProjectLoaded();
			});
		WaapiConnectionLostHandle = waapiClient->OnConnectionLost.AddLambda([this]()
			{
				WaapiDisconnected();
			});
	}
}
#endif

void UAkSettings::EnsureSoundDataPathIsInAlwaysCook() const
{
	if (!UseEventBasedPackaging)
	{
		return;
	}

	UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();

	bool packageSettingsNeedUpdate = false;
	FString soundDataGamePath = FString::Printf(TEXT("/Game/%s"), *WwiseSoundDataFolder.Path);

	if (!PackagingSettings->DirectoriesToAlwaysCook.ContainsByPredicate([soundDataGamePath](FDirectoryPath PathInArray) { return PathInArray.Path == soundDataGamePath; }))
	{
		FDirectoryPath newPath;
		newPath.Path = soundDataGamePath;

		PackagingSettings->DirectoriesToAlwaysCook.Add(newPath);
		packageSettingsNeedUpdate = true;
	}

	for (int32 i = PackagingSettings->DirectoriesToAlwaysCook.Num() - 1; i >= 0; --i)
	{
		FString ContentPath = PackagingSettings->DirectoriesToAlwaysCook[i].Path;
		if (ContentPath.IsEmpty()) 
		{
			continue;
		}

		if (ContentPath.StartsWith(TEXT("/"), ESearchCase::CaseSensitive))
		{
			// If this starts with /, this includes a root like /engine
			ContentPath = FPackageName::LongPackageNameToFilename(ContentPath / TEXT(""));
		}
		else
		{
			ContentPath = FPackageName::LongPackageNameToFilename(ContentPath);
		}
		
		if (!FPaths::DirectoryExists(ContentPath))
		{
			PackagingSettings->DirectoriesToAlwaysCook.RemoveAt(i);
			packageSettingsNeedUpdate = true;
		}
	}

	if (packageSettingsNeedUpdate)
	{
		AkUnrealHelper::SaveConfigFile(PackagingSettings);
	}
}

void UAkSettings::EnsurePluginContentIsInAlwaysCook() const
{
	UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();

	bool packageSettingsNeedUpdate = false;

	TArray<FString> PathsToCheck = { TEXT("/Wwise/WwiseTree"), TEXT("/Wwise/WwiseTypes") };

	for (auto pathToCheck : PathsToCheck)
	{
		if (!PackagingSettings->DirectoriesToAlwaysCook.ContainsByPredicate([pathToCheck](FDirectoryPath PathInArray) { return PathInArray.Path == pathToCheck; }))
		{
			FDirectoryPath newPath;
			newPath.Path = pathToCheck;

			PackagingSettings->DirectoriesToAlwaysCook.Add(newPath);
			packageSettingsNeedUpdate = true;
		}
	}

	if (packageSettingsNeedUpdate)
	{
		AkUnrealHelper::SaveConfigFile(PackagingSettings);
	}
}


void UAkSettings::RemoveSoundDataFromAlwaysCook(const FString& SoundDataPath)
{
	bool changed = false;

	UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();

	for (int32 i = PackagingSettings->DirectoriesToAlwaysCook.Num() - 1; i >= 0; --i)
	{
		if (PackagingSettings->DirectoriesToAlwaysCook[i].Path == SoundDataPath)
		{
			PackagingSettings->DirectoriesToAlwaysCook.RemoveAt(i);
			changed = true;
			break;
		}
	}

	if (changed)
	{
		AkUnrealHelper::SaveConfigFile(PackagingSettings);
	}
}

void UAkSettings::AddSoundDataToAlwaysStageAsUFS()
{
	UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();

	auto Settings = FAkPlatform::GetInitializationSettings();

	for(auto& platform : AkUnrealPlatformHelper::GetAllSupportedWwisePlatforms(true))
	{
		AkUnrealHelper::UpdateGeneratedSoundBanksFolder(*platform, PreviousWwiseSoundBankFolder);
	}
}

void UAkSettings::RemoveSoundDataFromAlwaysStageAsUFS(const FString& SoundDataPath)
{
	for (auto& platform : AkUnrealPlatformHelper::GetAllSupportedWwisePlatforms(true))
	{
		AkUnrealHelper::UpdateGeneratedSoundBanksFolder(*platform, SoundDataPath);
	}
}

void UAkSettings::SplitOrMergeMedia()
{
	TArray<FAssetData> MediaList;
	AkUEGetAssetsByClass(UAkMediaAsset::StaticClass(), MediaList, false);

	auto& AssetToolModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<FAssetRenameData> assetsToRename;

	for (auto& entry : MediaList)
	{
		if (auto mediaAsset = Cast<UAkMediaAsset>(entry.GetAsset()))
		{
			auto id = mediaAsset->Id;
			FString stringId = FString::Printf(TEXT("%u"), id);

			FString subFolder;

			if (SplitMediaPerFolder)
			{
				subFolder = FString::Printf(TEXT("%02x/%02x"), (id >> 24) & 0xFF, (id >> 16) & 0xFF);
			}

			FString mediaNewPath = ObjectTools::SanitizeObjectPath(FPaths::Combine(AkUnrealHelper::GetBaseAssetPackagePath(), AkUnrealHelper::MediaFolderName, subFolder, stringId + TEXT(".") + stringId));

			assetsToRename.Emplace(AkUEGetObjectPath(entry), FSoftObjectPath(mediaNewPath));
		}
	}

	AssetToolModule.Get().RenameAssets(assetsToRename);
}

#endif // WITH_EDITOR

const FAkAcousticTextureParams* UAkSettings::GetTextureParams(const uint32& shortID) const
{
	for (auto it = AcousticTextureParamsMap.CreateConstIterator(); it; ++it)
	{
		if (it.Value().shortID == shortID)
			return AcousticTextureParamsMap.Find(it.Key());
	}
	return nullptr;
}

bool UAkSettings::ReverbRTPCsInUse() const
{
	return DecayRTPCInUse() || DampingRTPCInUse() || PredelayRTPCInUse();
}

bool UAkSettings::DecayRTPCInUse() const
{
	const bool validPath = !DecayEstimateRTPC.ToSoftObjectPath().ToString().IsEmpty();
	return validPath || !DecayEstimateName.IsEmpty();
}

bool UAkSettings::DampingRTPCInUse() const
{
	const bool validPath = !HFDampingRTPC.ToSoftObjectPath().ToString().IsEmpty();
	return validPath || !HFDampingName.IsEmpty();
}

bool UAkSettings::PredelayRTPCInUse() const
{
	const bool validPath = !TimeToFirstReflectionRTPC.ToSoftObjectPath().ToString().IsEmpty();
	return validPath || !TimeToFirstReflectionName.IsEmpty();
}

bool UAkSettings::GetAssociatedAcousticTexture(const UPhysicalMaterial* physMaterial, UAkAcousticTexture*& acousticTexture) const
{
	TSoftObjectPtr<class UPhysicalMaterial> physMatPtr(physMaterial);
	auto props = AkGeometryMap.Find(physMatPtr);

	if (!props)
		return false;

	TSoftObjectPtr<class UAkAcousticTexture> texturePtr = props->AcousticTexture;
	acousticTexture = texturePtr.LoadSynchronous();
	return true;
}

bool UAkSettings::GetAssociatedOcclusionValue(const UPhysicalMaterial* physMaterial, float& occlusionValue) const
{
	TSoftObjectPtr<class UPhysicalMaterial> physMatPtr(physMaterial);
	auto props = AkGeometryMap.Find(physMatPtr);

	if (!props)
		return false;

	occlusionValue = props->OcclusionValue;
	return true;
}

void UAkSettings::GetAuxBusForDecayValue(float decay, UAkAuxBus*& auxBus)
{
	TArray<float> decayKeys;
	EnvironmentDecayAuxBusMap.GetKeys(decayKeys);
	decayKeys.Sort();
	int numKeys = decayKeys.Num();
	if (numKeys > 0)
	{
		int i = numKeys - 1;
		if (decay > decayKeys[i])
		{
			auxBus = DefaultReverbAuxBus.LoadSynchronous();
			return;
		}
		while (i > 0 && decay <= decayKeys[i - 1])
		{
			--i;
		}
		TSoftObjectPtr<UAkAuxBus> auxBusPtr = EnvironmentDecayAuxBusMap[decayKeys[i]];
		auxBus = auxBusPtr.LoadSynchronous();
	}
	else
	{
		auxBus = DefaultReverbAuxBus.LoadSynchronous();
	}
}

void UAkSettings::GetAudioInputEvent(UAkAudioEvent*& OutInputEvent)
{
	OutInputEvent = AudioInputEvent.LoadSynchronous();
}
