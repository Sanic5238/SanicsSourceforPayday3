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

#include "WaapiAssetSynchronizer.h"

#include "AkUnrealHelper.h"
#include "AkAcousticTexture.h"
#include "AkAssetDatabase.h"
#include "AkAudioBank.h"
#include "AkAudioEvent.h"
#include "AkAudioType.h"
#include "AkAuxBus.h"
#include "AkInitBank.h"
#include "AkRtpc.h"
#include "AkStateValue.h"
#include "AkSwitchValue.h"
#include "AkTrigger.h"
#include "AkFolder.h"
#include "AkWaapiUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetTools/Public/AssetToolsModule.h"
#include "Async/Async.h"
#include "ContentBrowser/Public/ContentBrowserModule.h"
#include "ContentBrowser/Public/IContentBrowserSingleton.h"
#include "Editor.h"
#include "Misc/MessageDialog.h"
#include "Misc/RedirectCollector.h"
#include "Misc/ScopedSlowTask.h"
#include "UnrealEd/Public/FileHelpers.h"
#include "AkAudio/Classes/AkSettingsPerUser.h"
#include "UnrealEd/Public/ObjectTools.h"

#define LOCTEXT_NAMESPACE "AkAssetFactory"

DEFINE_LOG_CATEGORY_STATIC(LogAkWaapiAssetSynchronizer, Log, All);

bool WaapiAssetSynchronizer::PauseAssetRegistryDelegates;

namespace WaapiAssetSynchronizer_Helper
{
	constexpr auto DeleteTime = 0.35f;
	constexpr auto SyncTime = 0.35f;

	void SubscribeWaapiCallback(FAkWaapiClient* waapiClient, const char* uri, WampEventCallback callback, uint64& subscriptionId, const TArray<FString>& ReturnArgs)
	{
		if (subscriptionId)
			return;

		TArray<TSharedPtr<FJsonValue>> ReturnArgsArray;
		for (auto& arg : ReturnArgs)
			ReturnArgsArray.Add(MakeShared<FJsonValueString>(arg));

		TSharedRef<FJsonObject> options = MakeShared<FJsonObject>();
		options->SetArrayField(WwiseWaapiHelper::RETURN, ReturnArgsArray);

		TSharedPtr<FJsonObject> result;
		waapiClient->Subscribe(uri, options, callback, subscriptionId, result);
	}

	void UnsubscribeWaapiCallback(FAkWaapiClient* waapiClient, uint64& subscriptionId)
	{
		if (subscriptionId == 0)
			return;

		TSharedPtr<FJsonObject> result = MakeShared<FJsonObject>();
		waapiClient->Unsubscribe(subscriptionId, result);
		subscriptionId = 0;
	}
}

void WaapiAssetSynchronizer::Init()
{
	auto waapiClient = FAkWaapiClient::Get();
	if (!waapiClient)
		return;

	subscribeWaapiCallbacks();

	projectLoadedHandle = waapiClient->OnProjectLoaded.AddRaw(this, &WaapiAssetSynchronizer::subscribeWaapiCallbacks);
	connectionLostHandle = waapiClient->OnConnectionLost.AddRaw(this, &WaapiAssetSynchronizer::unsubscribeWaapiCallbacks);
	clientBeginDestroyHandle = waapiClient->OnClientBeginDestroy.AddRaw(this, &WaapiAssetSynchronizer::unsubscribeWaapiCallbacks);

	auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	assetRemovedHandle = AssetRegistry.OnAssetRemoved().AddRaw(this, &WaapiAssetSynchronizer::onAssetRemoved);
	assetRenamedHandle = AssetRegistry.OnAssetRenamed().AddRaw(this, &WaapiAssetSynchronizer::onAssetRenamed);

	if (GEngine)
	{
		postEditorTickHandle = GEngine->OnPostEditorTick().AddRaw(this, &WaapiAssetSynchronizer::assetUpdateTimerTick);
	}
}

void WaapiAssetSynchronizer::Uninit()
{
	unsubscribeWaapiCallbacks();

	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		auto& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.OnAssetRemoved().Remove(assetRemovedHandle);
		AssetRegistry.OnAssetRenamed().Remove(assetRenamedHandle);
	}

	assetRemovedHandle.Reset();
	assetRenamedHandle.Reset();

	auto waapiClient = FAkWaapiClient::Get();
	if (waapiClient)
	{
		waapiClient->OnProjectLoaded.Remove(projectLoadedHandle);
		waapiClient->OnConnectionLost.Remove(connectionLostHandle);
		waapiClient->OnClientBeginDestroy.Remove(clientBeginDestroyHandle);
	}

	projectLoadedHandle.Reset();
	connectionLostHandle.Reset();
	clientBeginDestroyHandle.Reset();

	if (GEngine)
	{
		GEngine->OnPostEditorTick().Remove(postEditorTickHandle);
	}
}

void WaapiAssetSynchronizer::subscribeWaapiCallbacks()
{
	auto waapiClient = FAkWaapiClient::Get();
	if (!waapiClient)
		return;

	const TArray<FString> ReturnArgs
	{
		WwiseWaapiHelper::ID,
		WwiseWaapiHelper::NAME,
		WwiseWaapiHelper::PATH,
		WwiseWaapiHelper::TYPE,
		WwiseWaapiHelper::PARENT,
	};

	WaapiAssetSynchronizer_Helper::SubscribeWaapiCallback(waapiClient,
		ak::wwise::core::object::nameChanged,
		WampEventCallback::CreateRaw(this, &WaapiAssetSynchronizer::onRenamed),
		idRenamed,
		ReturnArgs);

	WaapiAssetSynchronizer_Helper::SubscribeWaapiCallback(waapiClient,
		ak::wwise::core::object::preDeleted,
		WampEventCallback::CreateRaw(this, &WaapiAssetSynchronizer::onPreDeleted),
		idPreDeleted,
		{
			WwiseWaapiHelper::ID,
			WwiseWaapiHelper::NAME,
			WwiseWaapiHelper::PATH,
			WwiseWaapiHelper::TYPE
		});

	WaapiAssetSynchronizer_Helper::SubscribeWaapiCallback(waapiClient,
		ak::wwise::core::object::childAdded,
		WampEventCallback::CreateRaw(this, &WaapiAssetSynchronizer::onChildAdded),
		idChildAdded,
		ReturnArgs);

	WaapiAssetSynchronizer_Helper::SubscribeWaapiCallback(waapiClient,
		ak::wwise::core::object::childRemoved,
		WampEventCallback::CreateRaw(this, &WaapiAssetSynchronizer::onChildRemoved),
		idChildRemoved,
		ReturnArgs);
}

void WaapiAssetSynchronizer::unsubscribeWaapiCallbacks()
{
	auto waapiClient = FAkWaapiClient::Get();
	if (!waapiClient)
		return;

	WaapiAssetSynchronizer_Helper::UnsubscribeWaapiCallback(waapiClient, idRenamed);
	WaapiAssetSynchronizer_Helper::UnsubscribeWaapiCallback(waapiClient, idPreDeleted);
	WaapiAssetSynchronizer_Helper::UnsubscribeWaapiCallback(waapiClient, idChildAdded);
	WaapiAssetSynchronizer_Helper::UnsubscribeWaapiCallback(waapiClient, idChildRemoved);
}

void WaapiAssetSynchronizer::assetUpdateTimerTick(float DeltaSeconds)
{
	if (deleteTimer > 0.f)
	{
		deleteTimer -= DeltaSeconds;

		if (deleteTimer <= 0.f)
		{
			if (assetsToDelete.Num() > 0)
			{
				auto copyAssetsToDelete = assetsToDelete;
				assetsToDelete.Empty();
				AkAssetDatabase::Get().DeleteAssets(copyAssetsToDelete);
			}
		}
	}

	if (syncTimer > 0.f)
	{
		syncTimer -= DeltaSeconds;

		if (syncTimer <= 0.f)
		{
			if (assetsToRename.Num() > 0)
			{
				auto copyAssetsToRename = assetsToRename;
				assetsToRename.Empty();
				renameAssets(copyAssetsToRename);
			}
		}
	}
}

void WaapiAssetSynchronizer::onPreDeleted(uint64_t Id, TSharedPtr<FJsonObject> Response)
{
	if (FAkAudioDevice::Get()->IsBuildingData)
		return;

	AsyncTask(ENamedThreads::GameThread, [this, Response]
		{
			const TSharedPtr<FJsonObject>* objectPtr = nullptr;
			if (!Response->TryGetObjectField(WwiseWaapiHelper::OBJECT, objectPtr) || !objectPtr)
				return;

			auto object = *objectPtr;

			FString id;
			if (!object->TryGetStringField(WwiseWaapiHelper::ID, id))
				return; // error parsing Json

			FGuid guidId;
			FGuid::ParseExact(id, EGuidFormats::DigitsWithHyphensInBraces, guidId);

			if (AkAssetDatabase::Get().AudioTypeMap.Find(guidId))
			{
				deleteTimer = WaapiAssetSynchronizer_Helper::DeleteTime;
				assetsToDelete.Add(guidId);
			}
		});
}

// There appears to be four different ways childAdded can be called, and it is intimately linked with renamed.
// SCENARIO #1: Use the "New" button in the corresponding hierarchy;
//		1.	Renamed is called first, with an empty old name, no parent, and an invalid path. We ignore this.
//		2.	ChildAdded is called, with correct information. We realize we have never seen this GUID before, 
//			so we push it on a "pending assets to create" stack. We then create the corresponding uasset 
//			immediately, removing the GUID from the stack.
// SCENARIO #2: Copy/paste an existing item
//		1.	ChildAdded is called first, but with the same name as the source item. We realize we have never 
//			seen this GUID before, so we push it on a "pending assets to create" stack. We then realize we 
//			already have an asset with that name, so we	wait for the rename event to give us the correct name.
//		2.	Rename is called with the correct name. We then create the asset, and remove the GUID from the stack.
// SCENARIO #3: Reparenting an object
//		1.	We only get a childAdded in that case. We realize we already know about that GUID, so we treat this 
//			as a rename.
// SCENARIO #4: Creating the item from the Actor-Mixer Hierarchy context menu (mostly New Event)
//		1.	ChildAdded gets called first, but with an empty name. We realize we have never seen this GUID before, 
//			so we push it on a "pending assets to create" stack. The empty name makes us exit this function early, 
//			waiting for the rename callback to be called.
//		2.	Renamed is called with the correct name. We then create the asset, and remove the GUID from the stack.
void WaapiAssetSynchronizer::onChildAdded(uint64_t Id, TSharedPtr<FJsonObject> Response)
{
	if (FAkAudioDevice::Get()->IsBuildingData)
		return;

	AsyncTask(ENamedThreads::GameThread, [this, Response]
		{
			const TSharedPtr<FJsonObject>* childObjectPtr = nullptr;
			if (!Response->TryGetObjectField(WwiseWaapiHelper::CHILD, childObjectPtr) || !childObjectPtr)
				return;

			auto& akAssetDatabase = AkAssetDatabase::Get();
			auto childObject = *childObjectPtr;

			FString id;
			if (!childObject->TryGetStringField(WwiseWaapiHelper::ID, id))
				return; // error parsing Json

			FGuid guidId;
			FGuid::ParseExact(id, EGuidFormats::DigitsWithHyphensInBraces, guidId);

			if (RenamesToIgnore.Contains(guidId))
			{
				RenamesToIgnore.Remove(guidId);
				return;
			}

			FString stringAssetType;
			if (!childObject->TryGetStringField(WwiseWaapiHelper::TYPE, stringAssetType))
				return; // error parsing Json

			const UClass* assetClass = WaapiAssetSynchronizer::GetClassByName(stringAssetType);
			if (akAssetDatabase.AudioTypeMap.Find(guidId) == nullptr && assetClass != nullptr)
			{
				// We don't know about this object. It will either be created here, or in a rename. 
				// Put it in a stack, so that rename knows to do an actual "create" in that case.
				akAssetDatabase.PendingAssetCreates.Add(guidId);
			}

			FString name;
			if (!childObject->TryGetStringField(WwiseWaapiHelper::NAME, name) || name.IsEmpty())
				return; // error parsing Json

			FString path;
			if (!childObject->TryGetStringField(WwiseWaapiHelper::PATH, path))
				return; // error parsing Json

			auto wwiseAssetType = WaapiAssetSynchronizer::GetTypeByName(stringAssetType);
			FString assetName = name;
			FGuid groupId;
			path = path.Replace(TEXT("\\"), TEXT("/"));
			if (assetClass == UAkStateValue::StaticClass() || assetClass == UAkSwitchValue::StaticClass())
			{
				const TSharedPtr<FJsonObject>* parentPtr = nullptr;
				if (!Response->TryGetObjectField(WwiseWaapiHelper::PARENT, parentPtr) || !parentPtr)
				{
					return; // error parsing Json
				}

				auto parent = *parentPtr;

				FString parentName;
				if (!parent->TryGetStringField(WwiseWaapiHelper::NAME, parentName))
				{
					return;
				}
				parentName = ObjectTools::SanitizeObjectPath(parentName);

				if (parentName.IsEmpty())
				{
					parentName = TEXT("NewStateGroup");
				}

				if (path.StartsWith(UnnamedStateGroupFolder))
				{
					path = path.Replace(*UnnamedStateGroupFolder, *AkAssetDatabase::TemporaryGroupValueFolder);
				}

				assetName = FString::Printf(TEXT("%s-%s"), *parentName, *name);
				auto stringGroupId = parent->GetStringField(WwiseWaapiHelper::ID);

				FGuid::ParseExact(stringGroupId, EGuidFormats::DigitsWithHyphensInBraces, groupId);
			}

			auto matchName = assetName;
			if (AkAssetDatabase::IsFolderType(wwiseAssetType))
			{
				matchName = ObjectTools::SanitizeObjectPath(name);
			}
			auto ExistingObjectsByName = akAssetDatabase.AudioTypeMap.FindByName(matchName);
			if (ExistingObjectsByName.Num() > 0)
			{
				for (auto ExistingObject : ExistingObjectsByName)
				{

					UClass* existingClass = ExistingObject.GetClass();
					if (existingClass == assetClass && akAssetDatabase.PendingAssetCreates.Contains(guidId))
					{
						//group values created in a copy operation will be lost if we don't create them, so we'll put them in a temp folder until the group is renamed
						if (wwiseAssetType == EWwiseItemType::State || wwiseAssetType == EWwiseItemType::Switch)
						{
							path = FPaths::Combine(AkAssetDatabase::TemporaryGroupValueFolder, path);
						}
						else
						{
							// Found a new object with an existing name, we need to create it in rename
							return;
						}
					}
				}
			}

			if (assetClass)
			{
				if (akAssetDatabase.AudioTypeMap.Find(guidId))
				{
					RenamesToIgnore.Add(guidId);
				}

				auto assetModified = false;
				if (AkAssetDatabase::IsFolderType(wwiseAssetType))
				{
					path = AkUnrealHelper::FormatFolderPath(path);
					akAssetDatabase.CreateOrRenameFolder(UAkFolder::StaticClass(), guidId, assetName, path, wwiseAssetType, AkAssetDatabase::AssetSyncMode::WAAPI, assetModified);
				}
				else
				{
					auto foundAsset = false;
					akAssetDatabase.CreateOrRenameAsset(assetClass, guidId, name, assetName, path, groupId, AkAssetDatabase::AssetSyncMode::WAAPI, foundAsset, assetModified);
				}
				if (assetModified)
				{
					if (stringAssetType == TEXT("StateGroup") || stringAssetType == TEXT("SwitchGroup"))
					{
						akAssetDatabase.RenameGroupValues(guidId, name, path);
					}
				}
				else
				{
					RenamesToIgnore.Remove(guidId);
				}
			}
		});
}

void WaapiAssetSynchronizer::onChildRemoved(uint64_t Id, TSharedPtr<FJsonObject> Response)
{
	if (FAkAudioDevice::Get()->IsBuildingData)
		return;

	AsyncTask(ENamedThreads::GameThread, [this, Response]
		{
			const TSharedPtr<FJsonObject>* childObjectPtr = nullptr;
			if (!Response->TryGetObjectField(WwiseWaapiHelper::CHILD, childObjectPtr) || !childObjectPtr)
				return;

			auto childObject = *childObjectPtr;

			FString stringId;
			if (!childObject->TryGetStringField(WwiseWaapiHelper::ID, stringId))
				return; // error parsing Json

			FGuid childId;
			FGuid::ParseExact(stringId, EGuidFormats::DigitsWithHyphensInBraces, childId);

			FString childAssetType;
			if (!childObject->TryGetStringField(WwiseWaapiHelper::TYPE, childAssetType))
				return; // error parsing Json

			FString childPath;
			if (!childObject->TryGetStringField(WwiseWaapiHelper::PATH, childPath))
				return; // error parsing Json
		});

}

void WaapiAssetSynchronizer::onRenamed(uint64_t Id, TSharedPtr<FJsonObject> Response)
{
	if (FAkAudioDevice::Get()->IsBuildingData)
		return;

	AsyncTask(ENamedThreads::GameThread, [this, Response]
		{
			const TSharedPtr<FJsonObject>* resultObjectPtr = nullptr;
			if (!Response->TryGetObjectField(WwiseWaapiHelper::OBJECT, resultObjectPtr) || !resultObjectPtr)
				return;
			auto resultObject = *resultObjectPtr;

			FString id;
			if (!resultObject->TryGetStringField(WwiseWaapiHelper::ID, id))
				return; // error parsing Json

			FGuid guidId;
			FGuid::ParseExact(id, EGuidFormats::DigitsWithHyphensInBraces, guidId);

			if (RenamesToIgnore.Contains(guidId))
			{
				RenamesToIgnore.Remove(guidId);
				return;
			}

			auto& akAssetDatabase = AkAssetDatabase::Get();
			FString oldName;
			Response->TryGetStringField(WwiseWaapiHelper::OLD_NAME, oldName);
			if (oldName.IsEmpty() && !akAssetDatabase.PendingAssetCreates.Contains(guidId))
			{
				// Invalid old name, and we're not trying to create this object, error happened
				return;
			}

			FString newName;
			Response->TryGetStringField(WwiseWaapiHelper::NEW_NAME, newName);

			FString path;
			if (!resultObject->TryGetStringField(WwiseWaapiHelper::PATH, path))
				return; // error parsing Json

			FString assetType;
			if (!resultObject->TryGetStringField(WwiseWaapiHelper::TYPE, assetType))
				return; // error parsing Json
			auto wwiseAssetType = WaapiAssetSynchronizer::GetTypeByName(assetType);

			FString assetName = newName;
			FGuid groupId;
			if (wwiseAssetType == EWwiseItemType::State || wwiseAssetType == EWwiseItemType::Switch)
			{
				const TSharedPtr<FJsonObject>* parentPtr = nullptr;
				if (!resultObject->TryGetObjectField(WwiseWaapiHelper::PARENT, parentPtr) || !parentPtr)
					return; // error parsing Json

				auto parent = *parentPtr;
				assetName = FString::Printf(TEXT("%s-%s"), *parent->GetStringField(WwiseWaapiHelper::NAME), *newName);

				auto stringGroupId = parent->GetStringField(WwiseWaapiHelper::ID);
				FGuid::ParseExact(stringGroupId, EGuidFormats::DigitsWithHyphensInBraces, groupId);
			}

			auto assetModified = false;
			if (wwiseAssetType == EWwiseItemType::StateGroup || wwiseAssetType == EWwiseItemType::SwitchGroup)
			{
				path = AkUnrealHelper::FormatFolderPath(path);

				akAssetDatabase.CreateOrRenameFolder(UAkFolder::StaticClass(), guidId, assetName, path.Replace(TEXT("\\"), TEXT("/")), wwiseAssetType, AkAssetDatabase::AssetSyncMode::WAAPI, assetModified);
				if (!assetModified)
				{
					RenamesToIgnore.Remove(guidId);
				}
				akAssetDatabase.RenameGroupValues(guidId, newName, path.Replace(TEXT("\\"), TEXT("/")));
			}
			else
			{
				const UClass* assetClass = WaapiAssetSynchronizer::GetClassByName(assetType);
				if (assetClass != nullptr)
				{
					if (akAssetDatabase.AudioTypeMap.Find(guidId))
					{
						RenamesToIgnore.Emplace(guidId);
					}
					if (AkAssetDatabase::IsFolderType(wwiseAssetType))
					{
						path = AkUnrealHelper::FormatFolderPath(path);
						akAssetDatabase.CreateOrRenameFolder(UAkFolder::StaticClass(), guidId, assetName, path.Replace(TEXT("\\"), TEXT("/")), wwiseAssetType, AkAssetDatabase::AssetSyncMode::WAAPI, assetModified);
					}
					else
					{
						auto assetFound = false;
						akAssetDatabase.CreateOrRenameAsset(assetClass, guidId, newName, assetName, path.Replace(TEXT("\\"), TEXT("/")), groupId, AkAssetDatabase::AssetSyncMode::WAAPI, assetFound, assetModified);
					}
					if (!assetModified)
					{
						RenamesToIgnore.Remove(guidId);
					}
				}
			}
		});
}

UClass* WaapiAssetSynchronizer::GetClassByName(const FString& stringAssetType)
{
	if (stringAssetType == TEXT("AuxBus"))
	{
		return UAkAuxBus::StaticClass();
	}
	else if (stringAssetType == TEXT("AcousticTexture"))
	{
		return UAkAcousticTexture::StaticClass();
	}
	else if (stringAssetType == TEXT("Event"))
	{
		return UAkAudioEvent::StaticClass();
	}
	else if (stringAssetType == TEXT("GameParameter"))
	{
		return UAkRtpc::StaticClass();
	}
	else if (stringAssetType == TEXT("State"))
	{
		return UAkStateValue::StaticClass();
	}
	else if (stringAssetType == TEXT("Switch"))
	{
		return UAkSwitchValue::StaticClass();
	}
	else if (stringAssetType == TEXT("Trigger"))
	{
		return UAkTrigger::StaticClass();
	}
	else if (stringAssetType == TEXT("StateGroup"))
	{
		return UAkFolder::StaticClass();
	}
	else if (stringAssetType == TEXT("SwitchGroup"))
	{
		return UAkFolder::StaticClass();
	}
	else if (stringAssetType == TEXT("WorkUnit"))
	{
		return UAkFolder::StaticClass();
	}
	else if (stringAssetType == TEXT("Folder"))
	{
		return UAkFolder::StaticClass();
	}
	return nullptr;
}

EWwiseItemType::Type WaapiAssetSynchronizer::GetTypeByName(const FString& stringAssetType)
{
	if (stringAssetType == TEXT("AuxBus"))
	{
		return EWwiseItemType::AuxBus;
	}
	else if (stringAssetType == TEXT("AcousticTexture"))
	{
		return  EWwiseItemType::AcousticTexture;
	}
	else if (stringAssetType == TEXT("Event"))
	{
		return EWwiseItemType::Event;
	}
	else if (stringAssetType == TEXT("GameParameter"))
	{
		return EWwiseItemType::GameParameter;
	}
	else if (stringAssetType == TEXT("State"))
	{
		return EWwiseItemType::State;
	}
	else if (stringAssetType == TEXT("StateGroup"))
	{
		return EWwiseItemType::StateGroup;
	}
	else if (stringAssetType == TEXT("Switch"))
	{
		return EWwiseItemType::Switch;
	}
	else if (stringAssetType == TEXT("SwitchGroup"))
	{
		return EWwiseItemType::SwitchGroup;
	}
	else if (stringAssetType == TEXT("Trigger"))
	{
		return EWwiseItemType::Trigger;
	}
	else if (stringAssetType == TEXT("WorkUnit"))
	{
		return EWwiseItemType::StandaloneWorkUnit;
	}
	else if (stringAssetType == TEXT("Folder"))
	{
		return EWwiseItemType::Folder;
	}
	return EWwiseItemType::None;
}

void WaapiAssetSynchronizer::onAssetRemoved(const FAssetData& RemovedAssetData)
{
	if (auto AkSettingsPerUser = GetDefault<UAkSettingsPerUser>())
	{
		if (!AkSettingsPerUser->bAutoConnectToWAAPI) return;
	}
	if (PauseAssetRegistryDelegates)
	{
		return;
	}

	auto waapiClient = FAkWaapiClient::Get();
	if (!waapiClient)
		return;

	if (!FAkWaapiClient::IsProjectLoaded())
	{
		return;
	}

	auto assetInstance = Cast<UAkAudioType>(RemovedAssetData.GetAsset());
	if (!assetInstance)
	{
		return;
	}

	if (!assetInstance->ID.IsValid())
	{
		return;
	}

	if (!AkAssetDatabase::Get().Remove(assetInstance))
	{
		return;
	}

	TSharedRef<FJsonObject> args = MakeShared<FJsonObject>();
	args->SetStringField(WwiseWaapiHelper::OBJECT, assetInstance->ID.ToString(EGuidFormats::DigitsWithHyphensInBraces));
	TSharedRef<FJsonObject> options = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> result;
	if (!waapiClient->Call(ak::wwise::core::object::delete_, args, options, result))
	{
		UE_LOG(LogAkWaapiAssetSynchronizer, Error, TEXT("Unable to delete <%s> from path <%s> due to WAAPI error"), *RemovedAssetData.AssetName.GetPlainNameString(), *AkUEGetObjectPathString(RemovedAssetData));
	}
}

void WaapiAssetSynchronizer::onAssetRenamed(const FAssetData& NewAssetData, const FString& OldPath)
{
	if (auto AkSettingsPerUser = GetDefault<UAkSettingsPerUser>())
	{
		if (!AkSettingsPerUser->bAutoConnectToWAAPI) return;
	}
	const auto ID = NewAssetData.GetTagValueRef<FGuid>(GET_MEMBER_NAME_CHECKED(UAkAudioType, ID));
	if (!ID.IsValid())
	{
		return;
	}
	if (RenamesToIgnore.Contains(ID))
	{
		RenamesToIgnore.Remove(ID);
		return;
	}
	if (PauseAssetRegistryDelegates)
	{
		RenamesToIgnore.Remove(ID);
		return;
	}
	if (NewAssetData.GetClass()->IsChildOf<UAkGroupValue>() || NewAssetData.GetClass()->IsChildOf<UAkAudioBank>())
	{
		return;
	}

	TArray<FString> splitPath;
	AkUEGetObjectPathString(NewAssetData).ParseIntoArray(splitPath, TEXT("/"), true);
	int newDepth = splitPath.Num();
	OldPath.ParseIntoArray(splitPath, TEXT("/"), true);

	assetsToRename.Add({ NewAssetData.GetClass()->IsChildOf<UAkFolder>(), newDepth, splitPath.Num(), ID, NewAssetData, OldPath, NewAssetData.PackagePath.ToString() });
	syncTimer = WaapiAssetSynchronizer_Helper::SyncTime;
}

void WaapiAssetSynchronizer::renameAssets(TArray<FRenameData>& renamedAssets)
{
	//Sort assets by folder depth and then by path depth
	renamedAssets.Sort(
		[](const FRenameData& a, const  FRenameData& b) {
			if (a.IsFolder && !b.IsFolder) return true;
			else if (!a.IsFolder && b.IsFolder) return false;
			else return a.NewPathDepth < b.NewPathDepth;
		}
	);

	struct MovedFolderInfo {
		FString oldpath;
		FString newPath;
	};

	TArray<FRenameData> failedRenames;
	TArray<MovedFolderInfo> movedFolders;
	TArray<FString> failedFolders;

	for (auto renameData : renamedAssets)
	{
		bool skip = false;
		for (auto failedFolder : failedFolders)
		{
			if (renameData.NewPath.StartsWith(failedFolder))
			{
				failedRenames.Add(renameData);
				skip = true;
				break;
			}
		}

		for (auto movedFolder : movedFolders)
		{
			if (renameData.OldPath.StartsWith(movedFolder.oldpath) && renameData.NewPath.StartsWith(movedFolder.newPath))
			{
				skip = true;
				break;
			}
		}
		if (skip) continue;

		bool renameSucceeded = renameAsset(renameData);
		if (!renameSucceeded)
		{
			failedRenames.Add(renameData);
			if (renameData.AssetData.GetClass()->IsChildOf<UAkFolder>())
			{
				failedFolders.Add(renameData.NewPath);
			}
		}
		else
		{
			if (renameData.AssetData.GetClass()->IsChildOf<UAkFolder>())
			{
				movedFolders.Add({ FPaths::GetPath(renameData.OldPath), FPaths::GetPath(renameData.AssetData.PackagePath.ToString()) });
			}
		}
	}
	if (failedRenames.Num() > 0)
	{
		undoAssetRenames(failedRenames);
	}
}

bool WaapiAssetSynchronizer::renameAsset(const FRenameData& RenameData)
{
	bool bWaapiAvailable = true;
	FString waapiErrorMessage;
	auto WaapiClient = FAkWaapiClient::Get();
	if (!WaapiClient || !WaapiClient->IsConnected())
	{
		bWaapiAvailable = false;
		waapiErrorMessage = TEXT("WAAPI client is not connected");
	}

	if (!FAkWaapiClient::IsProjectLoaded())
	{
		bWaapiAvailable = false;
		waapiErrorMessage = TEXT("Project is not loaded in Wwise Authoring");
	}

	if (!bWaapiAvailable)
	{
		const FFormatNamedArguments Args
		{
			{ TEXT("OldPath"), FText::FromString(RenameData.OldPath) },
			{ TEXT("Reason"), FText::FromString(waapiErrorMessage) }
		};

		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("AkWaapiAssetSynchronizer", "CannotModifyAsset", "Cannot modify asset '{OldPath}'. Reason: {Reason}"), Args));
		return false;
	}

	auto NewAssetPath = AkUEGetObjectPathString(RenameData.AssetData);

	FString OldDirectory;
	FString OldFileName;
	FString OldExtension;
	FPaths::Split(RenameData.OldPath, OldDirectory, OldFileName, OldExtension);

	FString NewDirectory;
	FString NewFileName;
	FString NewExtension;
	FPaths::Split(NewAssetPath, NewDirectory, NewFileName, NewExtension);

	TSharedRef<FJsonObject> Options = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> Result;

	FString GUIDString = RenameData.Id.ToString(EGuidFormats::DigitsWithHyphensInBraces);
	bool bRenameSucceeded = true;
	FString SetNameErrorMessage;
	bool bIsFolderAsset = RenameData.AssetData.GetClass()->IsChildOf<UAkFolder>();

	if (bIsFolderAsset)
	{
		NewFileName = FPaths::GetBaseFilename(NewDirectory);
	}

	if (NewFileName != OldFileName)
	{
		TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>();
		Args->SetStringField(WwiseWaapiHelper::OBJECT, GUIDString);
		Args->SetStringField(WwiseWaapiHelper::VALUE, NewFileName);
		RenamesToIgnore.Add(RenameData.Id);
		bRenameSucceeded = WaapiClient->Call(ak::wwise::core::object::setName, Args, Options, Result);
		if (!bRenameSucceeded)
		{
			RenamesToIgnore.Remove(RenameData.Id);
			Result->TryGetStringField(TEXT("message"), SetNameErrorMessage);
		}
	}

	bool bMoveSucceeded = true;
	FString MoveErrorMessage;
	if (NewDirectory != OldDirectory)
	{
		bool bIsValidDestination = false;
		//make sure new parent directory is in folder heirarchy
		auto ParentPath = NewAssetPath;
		if (bIsFolderAsset)
		{
			ParentPath = NewDirectory;
		}
		if (AkAssetDatabase::Get().IsInWwiseFolderHeirarchy(ParentPath))
		{
			AkAssetDatabase::Get().UpdateWwiseFolderPaths(NewAssetPath);
			TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>();
			Args->SetStringField(WwiseWaapiHelper::PARENT, AkAssetDatabase::Get().GetFolderGuid(ParentPath).ToString(EGuidFormats::DigitsWithHyphensInBraces));

			Args->SetStringField(WwiseWaapiHelper::OBJECT, GUIDString);
			RenamesToIgnore.Add(RenameData.Id);
			bMoveSucceeded = WaapiClient->Call(ak::wwise::core::object::move, Args, Options, Result);
			if (!bMoveSucceeded)
			{
				RenamesToIgnore.Remove(RenameData.Id);
				Result->TryGetStringField(TEXT("message"), MoveErrorMessage);
			}
		}
		else
		{
			bMoveSucceeded = false;
			MoveErrorMessage = TEXT("Parent folders were not in wwise heirarchy");
		}
	}

	//This is the case where a valid asset change has been caused by changes within the unreal project
	if (bRenameSucceeded && bMoveSucceeded)
	{
		return true;
	}

	if (!bRenameSucceeded)
	{
		const FFormatNamedArguments Args
		{
			{ TEXT("OldName"), FText::FromString(OldFileName) },
			{ TEXT("NewName"), FText::FromString(NewFileName) },
			{ TEXT("Reason"), FText::FromString(SetNameErrorMessage) }
		};

		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("AkWaapiAssetSynchronizer", "CannotRenameAsset", "Cannot rename asset from '{OldName}' to '{NewName}'. Reason: {Reason}."), Args));
	}

	if (!bMoveSucceeded)
	{
		const FFormatNamedArguments Args
		{
			{ TEXT("OldName"), FText::FromString(OldFileName) },
			{ TEXT("OldPath"), FText::FromString(OldDirectory) },
			{ TEXT("NewPath"), FText::FromString(NewDirectory) },
			{ TEXT("Reason"), FText::FromString(MoveErrorMessage) }
		};

		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("AkWaapiAssetSynchronizer", "CannotMoveAsset", "Cannot move '{OldName}' from '{OldPath}' to '{NewPath}'. Reason: {Reason}."), Args));
	}

	return false;
}

void WaapiAssetSynchronizer::undoAssetRenames(const TArray<FRenameData>& RenamedAssets)
{
	TArray<FAssetRenameData> AssetsToRevert;
	TArray<FGuid> IgnoredIds;
	for (FRenameData RenameData : RenamedAssets)
	{
		const auto ID = RenameData.AssetData.GetTagValueRef<FGuid>(GET_MEMBER_NAME_CHECKED(UAkAudioType, ID));
		const auto newPath = AkUEGetObjectPathString(RenameData.AssetData);
		AkAssetDatabase::Get().IgnoreDeleteOnAssetMove(ID);
		AkAssetDatabase::Get().IgnoreAddOnAssetMove(ID);

		RenamesToIgnore.Add(ID);
		IgnoredIds.Add(ID);
		AkUEAddAssetPathRedirection(RenameData.OldPath, newPath);
		AssetsToRevert.Add({ AkUEGetObjectPath(RenameData.AssetData), FSoftObjectPath(RenameData.OldPath) });
	}

	const auto& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (!AssetToolsModule.Get().RenameAssets(AssetsToRevert))
	{
		for (auto ID : IgnoredIds)
		{
			RenamesToIgnore.Remove(ID);
		}
	}

	TArray< FAssetData> assetsToSync;
	for (auto asset : RenamedAssets)
	{
		assetsToSync.Add(FAssetData(FSoftObjectPath(asset.OldPath).TryLoad()));
	}

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(assetsToSync);
}


void WaapiAssetSynchronizer::undoAssetRename(const FAssetData& NewAssetData, const FString& OldPath)
{
	const FGuid ID = NewAssetData.GetTagValueRef<FGuid>(GET_MEMBER_NAME_CHECKED(UAkAudioType, ID));
	const FString NewAssetPath = AkUEGetObjectPathString(NewAssetData);

	RenamesToIgnore.Add(ID);

	// This is required to avoid an assert in RenameAssets() when the asset is referenced somewhere
	AkUEAddAssetPathRedirection(OldPath,NewAssetPath);

	const FAssetRenameData AssetRenameData{ NewAssetData.ToSoftObjectPath(), FSoftObjectPath(OldPath) };

	const auto& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (!AssetToolsModule.Get().RenameAssets(TArray<FAssetRenameData>{AssetRenameData}))
	{
		RenamesToIgnore.Remove(ID);
	}

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets({ FAssetData(FSoftObjectPath(OldPath).TryLoad()) });
}

#undef LOCTEXT_NAMESPACE
