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


#include "Platforms/AkPlatform_Switch/AkSwitchInitializationSettings.h"
#include "AkAudioDevice.h"
#include "Misc/Paths.h"

#if PLATFORM_SWITCH
#include "Generated/AkSwitchPlugins.h"
#if AK_SUPPORT_OPUS
#include <AK/Plugin/AkOpusNXFactory.h>
#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Memory hooks

#if PLATFORM_SWITCH && (UE_BUILD_SHIPPING || UE_BUILD_TEST)
namespace AkLowLevelMemory
{
	void* Alloc(size_t Size)
	{
		Size = Align(Size, nn::os::MemoryPageSize);
		return FMemory::Malloc(Size, nn::os::MemoryPageSize);
	}

	void Free(void* Addr, size_t Size)
	{
		FMemory::Free(Addr);
	}
}
#endif // PLATFORM_SWITCH

//////////////////////////////////////////////////////////////////////////
// UAkSwitchInitializationSettings

UAkSwitchInitializationSettings::UAkSwitchInitializationSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CommunicationSettings.DiscoveryBroadcastPort = FAkCommunicationSettings::DefaultDiscoveryBroadcastPort;
	CommunicationSettings.CommandPort = FAkCommunicationSettings::DefaultDiscoveryBroadcastPort + 1;
	CommunicationSettings.NotificationPort = FAkCommunicationSettings::DefaultDiscoveryBroadcastPort + 2;
}

void UAkSwitchInitializationSettings::FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const
{
#if PLATFORM_SWITCH
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	InitializationStructure.SetupLLMAllocFunctions(AkLowLevelMemory::Alloc, AkLowLevelMemory::Free);
#else
	InitializationStructure.SetupLLMAllocFunctions();
#endif
#endif

	CommonSettings.FillInitializationStructure(InitializationStructure);
	CommunicationSettings.FillInitializationStructure(InitializationStructure);
	AdvancedSettings.FillInitializationStructure(InitializationStructure);

#if PLATFORM_SWITCH
	InitializationStructure.PlatformInitSettings.uSampleRate = CommonSettings.SampleRate;
	InitializationStructure.DeviceSettings.threadProperties.uStackSize = 1 * 1024 * 1024;
#if AK_ENABLE_COMMUNICATION
	InitializationStructure.CommSettings.bInitSystemLib = false;
#endif
#endif
}
