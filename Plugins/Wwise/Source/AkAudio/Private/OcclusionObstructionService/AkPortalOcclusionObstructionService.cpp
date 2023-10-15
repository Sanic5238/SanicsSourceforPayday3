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


/*=============================================================================
AkOcclusionObstructionService.cpp:
=============================================================================*/

#include "OcclusionObstructionService/AkPortalOcclusionObstructionService.h"
#include "OcclusionObstructionService/AkOcclusionObstructionService.h"
#include "AkAudioDevice.h"
#include "AkAcousticPortal.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/PrimitiveComponent.h"


void AkPortalOcclusionObstructionService::Init(UAkPortalComponent* in_portal, float in_refreshInterval)
{
	_Init(in_portal->GetWorld(), in_refreshInterval);
	AssociatedPortal = in_portal;
}
void AkPortalOcclusionObstructionService::SetOcclusionObstruction(AkGameObjectID ListenerId, float Value)
{
	AK::SpatialAudio::SetPortalObstructionAndOcclusion(AssociatedPortal->GetPortalID(), Value, 0.0f);
}
