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

#include "Platforms/AkPlatformInfo.h"
#include "AkXboxOnePlatformInfo.generated.h"

UCLASS()
class UAkXboxOnePlatformInfo : public UAkPlatformInfo
{
	GENERATED_BODY()

public:
	UAkXboxOnePlatformInfo()
	{
		WwisePlatform = "XboxOne";

#ifdef AK_XBOXONE_VS_VERSION
		Architecture = "XboxOne_" AK_XBOXONE_VS_VERSION;
#else
		Architecture = "XboxOne_vc140";
#endif

		LibraryFileNameFormat = "{0}.dll";
		DebugFileNameFormat = "{0}.pdb";

#if WITH_EDITORONLY_DATA
		UAkPlatformInfo::UnrealNameToPlatformInfo.Add("XboxOne", this);
#endif
	}
};
