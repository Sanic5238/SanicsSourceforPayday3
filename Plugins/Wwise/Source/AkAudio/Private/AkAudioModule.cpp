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


#include "AkAudioModule.h"
#include "AkAudioDevice.h"
#include "AkSettings.h"
#include "AkAudioStyle.h"
#include "AkWaapiClient.h"
#include "Misc/CoreDelegates.h"

#if WITH_EDITOR
#include "AkUnrealHelper.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#endif

IMPLEMENT_MODULE( FAkAudioModule, AkAudio )
FAkAudioModule* FAkAudioModule::AkAudioModuleInstance = nullptr;

void FAkAudioModule::StartupModule()
{
	if (AkAudioModuleInstance)
		return;

	AkAudioModuleInstance = this;

#if WITH_EDITOR
	TArray<FString> paths;
	IFileManager::Get().FindFilesRecursive(paths, *FPaths::ProjectContentDir(), TEXT("InitBank.uasset"), true, false);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().ScanFilesSynchronous(paths, false);
#endif

	AkAudioDevice = new FAkAudioDevice;
	if (!AkAudioDevice)
		return;

	if (FAkAudioDevice::IsAudioAllowed())
	{
		FAkWaapiClient::Initialize();
	}

	if (!AkAudioDevice->Init())
	{
		delete AkAudioDevice;
		AkAudioDevice = nullptr;
		return;
	}

	OnTick = FTickerDelegate::CreateRaw(AkAudioDevice, &FAkAudioDevice::Update);
	TickDelegateHandle = FCoreTickerType::GetCoreTicker().AddTicker(OnTick);

#if WITH_EDITOR
	FAkAudioStyle::Initialize();
#if AK_SUPPORT_WAAPI
	FAkWaapiClient::Initialize();
	if (UAkSettings* AkSettings = GetMutableDefault<UAkSettings>())
	{
		AkSettings->InitWaapiSync();
	}
#endif
#endif
}

void FAkAudioModule::ShutdownModule()
{
	FCoreTickerType::GetCoreTicker().RemoveTicker(TickDelegateHandle);

	if (AkAudioDevice) 
	{
		AkAudioDevice->Teardown();
		delete AkAudioDevice;
		AkAudioDevice = nullptr;
	}

#if WITH_EDITOR
	FAkAudioStyle::Shutdown();
#if AK_SUPPORT_WAAPI
	FAkWaapiClient::DeleteInstance();
#endif
#endif

	AkAudioModuleInstance = nullptr;
}

FAkAudioDevice* FAkAudioModule::GetAkAudioDevice()
{ 
	return AkAudioDevice;
}
