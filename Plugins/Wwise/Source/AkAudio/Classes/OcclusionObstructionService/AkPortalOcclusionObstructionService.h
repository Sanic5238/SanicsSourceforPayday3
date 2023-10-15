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
AkPortalOcclusionObstructionService.h:
=============================================================================*/

#pragma once

#include "AkInclude.h"
#include "AkAudioDevice.h"
#include "WorldCollision.h"
#include "HAL/ThreadSafeBool.h"
#include "OcclusionObstructionService/AkOcclusionObstructionService.h"

class AActor;
class UAkPortalComponent;

class AkPortalOcclusionObstructionService : public AkOcclusionObstructionService
{
public:
	void Init(UAkPortalComponent* in_portalId, float in_refreshInterval);
	
	virtual void SetOcclusionObstruction(AkGameObjectID ListenerID, float Value);

	virtual ~AkPortalOcclusionObstructionService() {}

private:
	UAkPortalComponent * AssociatedPortal;
};
