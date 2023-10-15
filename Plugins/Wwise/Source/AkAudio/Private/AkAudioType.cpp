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

#include "AkAudioType.h"

#include "Async/Async.h"
#include "AkAudioModule.h"
#include "AkAudioDevice.h"
#include "AkGroupValue.h"
#include "AkFolder.h"
#include "Core/Public/Modules/ModuleManager.h"

void UAkAudioType::PostLoad()
{
	Super::PostLoad();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	UE_LOG(LogAkAudio, VeryVerbose, TEXT("UAkAudioType: PostLoad. Name %s - Short ID '%u'."), *GetName(), ShortID);
	if (FModuleManager::Get().IsModuleLoaded(TEXT("AkAudio")))
	{
		LoadBank();
	}
	else
	{
		FAkAudioDevice::DelayAssetLoad(this);
	}
}

void UAkAudioType::MarkDirtyInGameThread()
{
#if WITH_EDITOR
	AsyncTask(ENamedThreads::GameThread, [this] 
		{
			MarkPackageDirty();
		});
#endif
}

bool UAkAudioType::ShortIdMatchesName(AkUInt32& OutIdFromName)
{
	if (auto AudioDevice = FAkAudioDevice::Get())
	{
		if (IsA<UAkGroupValue>())
		{
			FString ValueName;
			GetName().Split(TEXT("-"), nullptr, &ValueName);
			OutIdFromName = FAkAudioDevice::GetIDFromString(ValueName);
		}
		else
		{
			OutIdFromName = FAkAudioDevice::GetIDFromString(GetName());
		}

		if (ShortID != OutIdFromName)
		{
			//Folder asset name does not correspond to actual wwise object name and we don't use the short ID anyway
			if (IsA<UAkFolder>())
			{
				return true;
			}
			UE_LOG(LogAkAudio, Warning, TEXT("%s - Current Short ID '%u' is different from expected ID '%u'"), *GetName(), ShortID, OutIdFromName);
			return false;
		}
	}
	return true;
}

void UAkAudioType::LoadBank()
{
	ValidateShortId(false);
}

void UAkAudioType::ValidateShortId(bool bMarkAsDirty)
{
	AkUInt32 IdFromName;
	if (!ShortIdMatchesName(IdFromName))
	{
		if (bMarkAsDirty)
		{
			ShortID = IdFromName;
			MarkDirtyInGameThread();
		}
	}
}

#if WITH_EDITOR
void UAkAudioType::Reset(TArray<FAssetData>& InOutAssetsToDelete)
{
	if (ShortID != 0)
	{
		ShortID = 0;
		bChangedDuringReset = true;
	}

	if (bChangedDuringReset)
	{
		bChangedDuringReset = false;
		MarkDirtyInGameThread();
	}
}
#endif
