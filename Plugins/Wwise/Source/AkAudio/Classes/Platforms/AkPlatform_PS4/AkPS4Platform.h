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

#if PLATFORM_PS4

#include "Platforms/AkPlatformBase.h"
#include "AkPS4InitializationSettings.h"

#define TCHAR_TO_AK(Text) (const ANSICHAR*)(TCHAR_TO_ANSI(Text))

using UAkInitializationSettings = UAkPS4InitializationSettings;

struct AKAUDIO_API FAkPS4Platform : FAkPlatformBase
{
	static const UAkInitializationSettings* GetInitializationSettings()
	{
		return GetDefault<UAkPS4InitializationSettings>();
	}

	static const FString GetPlatformBasePath()
	{
		return FString("PS4");
	}

	static FString GetWwisePluginDirectory();
	static FString GetDSPPluginsDirectory(const FString& PlatformArchitecture);
};

using FAkPlatform = FAkPS4Platform;

#endif
