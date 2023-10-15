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

#include "AkInclude.h"
#include "AkAudioDevice.h"
#include "Templates/Function.h"

/*------------------------------------------------------------------------------------
AkAudioInput Delegates
------------------------------------------------------------------------------------*/

DECLARE_DELEGATE_RetVal_ThreeParams(bool, FAkGlobalAudioInputDelegate, uint32, uint32, float**);
DECLARE_DELEGATE_OneParam(FAkGlobalAudioFormatDelegate, AkAudioFormat&);

/*------------------------------------------------------------------------------------
FAkAudioInputManager
------------------------------------------------------------------------------------*/

class AKAUDIO_API FAkAudioInputManager
{
public:

    /**
     * Post an event to ak soundengine
     *
     * @param Event Name of the event to post
     * @param Actor Actor on which to play the event
     * @param AudioSamplesCallback Callback that fills the audio samples buffer
     * @param AudioFormatCallback Callback that sets the audio format
     * @param GainCallback Callback that returns the gain level for the audio input
     * @return ID assigned by ak soundengine
     */
    static AkPlayingID PostAudioInputEvent(
	    UAkAudioEvent* Event,
	    AActor* Actor,
	    FAkGlobalAudioInputDelegate AudioSamplesDelegate,
	    FAkGlobalAudioFormatDelegate AudioFormatDelegate,
        EAkAudioContext AudioContext = EAkAudioContext::Foreign
    );

    /**
     * Post an event to ak soundengine by name
     *
     * @param EventName Name of the event to post
     * @param Actor Actor on which to play the event
     * @param AudioSamplesCallback Callback that fills the audio samples buffer
     * @param AudioFormatCallback Callback that sets the audio format
     * @param GainCallback Callback that returns the gain level for the audio input
     * @return ID assigned by ak soundengine
     */
    static AkPlayingID PostAudioInputEvent(
        const FString& EventName,
        AActor * Actor,
        FAkGlobalAudioInputDelegate AudioSamplesDelegate,
        FAkGlobalAudioFormatDelegate AudioFormatDelegate
    );

    /**
     * Post an event to ak soundengine by name
     *
     * @param EventName Name of the event to post
     * @param Component AkComponent on which to play the event
     * @param AudioSamplesCallback Callback that fills the audio samples buffer
     * @param AudioFormatCallback Callback that sets the audio format
     * @param GainCallback Callback that returns the gain level for the audio input
     * @return ID assigned by ak soundengine
     */
    static AkPlayingID PostAudioInputEvent(
        const FString& EventName,
        UAkComponent* Component,
        FAkGlobalAudioInputDelegate AudioSamplesDelegate,
        FAkGlobalAudioFormatDelegate AudioFormatDelegate
    );

	/**
	 * Post an event to ak soundengine by name
	 *
	 * @param EventName Name of the event to post
	 * @param AkGameObjecton which to play the event
	 * @param AudioSamplesCallback Callback that fills the audio samples buffer
	 * @param AudioFormatCallback Callback that sets the audio format
	 * @param GainCallback Callback that returns the gain level for the audio input
	 * @return ID assigned by ak soundengine
	 */
	static AkPlayingID PostAudioInputEvent(
		const FString& EventName,
		AkGameObjectID GameObject,
		FAkGlobalAudioInputDelegate AudioSamplesDelegate,
		FAkGlobalAudioFormatDelegate AudioFormatDelegate
	);

};