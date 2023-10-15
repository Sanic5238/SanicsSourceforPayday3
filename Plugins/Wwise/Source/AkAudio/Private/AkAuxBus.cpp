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

#include "AkAuxBus.h"

UAkAssetData* UAkAuxBus::CreateAssetData(UObject* Parent) const
{
	return NewObject<UAkAssetData>(Parent);
}

#if WITH_EDITOR
UAkAssetData* UAkAuxBus::FindOrAddAssetData(const FString& Platform, const FString& Language)
{
	{
		FScopeLock autoLock(&AssetDataLock);

		//Make sure this asset no can no longer reference aux bus media (which is already referenced by the Init bank)
		if (PlatformAssetData && PlatformAssetData->AssetDataPerPlatform.Contains(Platform))
		{
			if (auto* PlatformData = Cast<UAkAssetDataWithMedia>(PlatformAssetData->AssetDataPerPlatform[Platform]))
			{
				PlatformAssetData->AssetDataPerPlatform.Remove(Platform);
			}
		}
	}

	return UAkAssetBase::FindOrAddAssetData(Platform, Language);
}
#endif