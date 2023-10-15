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


#include "AkInitBank.h"

#if WITH_EDITOR
FOnInitBankChanged UAkInitBank::OnInitBankChanged;
#endif

UAkInitBank::UAkInitBank()
{
	bDelayLoadAssetData = true;
}

UAkAssetData* UAkInitBank::CreateAssetData(UObject* parent) const
{
	return NewObject<UAkInitBankAssetData>(parent);
}

void UAkInitBank::LoadBank()
{
	Super::LoadBank();
	bDelayLoadAssetData = false;
#if WITH_EDITOR
	FAkAudioDevice* AkAudioDevice = FAkAudioDevice::Get();
	if (AkAudioDevice && AkAudioDevice->InitBank != this)
	{
		AkAudioDevice->InitBank = this;
		AddToRoot();
		OnInitBankChanged.ExecuteIfBound(this);
	}
#endif
}

#if WITH_EDITOR
void UAkInitBank::Reset(TArray<FAssetData>& InOutAssetsToDelete)
{
	if (AvailableAudioCultures.Num() > 0)
	{
		bChangedDuringReset = true;
	}
	AvailableAudioCultures.Empty();

	// ALWAYS call Super::Reset() last, since it will check if things have been modified
	// before marking as dirty.
	Super::Reset(InOutAssetsToDelete);
}
#endif
