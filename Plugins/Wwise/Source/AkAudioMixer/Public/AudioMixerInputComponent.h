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

#include "CoreMinimal.h"
#include "AkInclude.h"
#include "AkAudioInputManager.h"

// FIXME Rename this class
class FAudioMixerInputComponent
{

public:
	FAudioMixerInputComponent();
	~FAudioMixerInputComponent();


	bool FillSamplesBuffer(uint32 NumChannels, uint32 NumSamples, float** BufferToFill);
	/** This callback is used to provide the Wwise sound engine with the required audio format. */
	void GetChannelConfig(AkAudioFormat& OutAudioFormat);

	AkPlayingID PostAssociatedAudioInputEvent(class UAkAudioEvent* InputEvent);
	void PostUnregisterGameObject();

	FAkGlobalAudioInputDelegate OnNextBuffer;

private:
	AkGameObjectID GetAkGameObjectID() const;

	AkPlayingID PlayingID;

};
