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

#include "Engine/EngineTypes.h"
#include "CoreUObject/Public/UObject/StrongObjectPtr.h"

class UAkCallbackInfo;

class AkCallbackInfoPool final
{
public:
	template<typename CallbackType>
	CallbackType* Acquire()
	{
		return static_cast<CallbackType*>(internalAcquire(CallbackType::StaticClass()));
	}

	void Release(UAkCallbackInfo* instance);

private:
	UAkCallbackInfo* internalAcquire(UClass* type);

private:
	TMap<UClass*, TArray<UAkCallbackInfo*>> Pool;
	TArray<TStrongObjectPtr<UAkCallbackInfo>> gcStorage;
};
