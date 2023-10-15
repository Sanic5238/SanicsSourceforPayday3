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

#include "CookAkSoundDataTask.h"

#include "Settings/EditorLoadingSavingSettings.h"
#include "ToolBehavior/AkToolBehavior.h"
#include "Async/Async.h"

CookAkSoundDataTask::CookAkSoundDataTask(const AkSoundDataBuilder::InitParameters& InitParameters)
{
	_dataSource = AkToolBehavior::Get()->CookAkSoundDataTask_CreateBuilder(InitParameters);
	_dataSource->Init();
}

CookAkSoundDataTask::~CookAkSoundDataTask()
{
}

void CookAkSoundDataTask::ExecuteForEditorPlatform()
{
	AkSoundDataBuilder::InitParameters initParameters;
	initParameters.Platforms = { FPlatformProperties::IniPlatformName() };

	ExecuteTask(initParameters);
}

void CookAkSoundDataTask::ExecuteTask(const AkSoundDataBuilder::InitParameters& InitParameters)
{

	AsyncTask(ENamedThreads::Type::AnyBackgroundThreadNormalTask, [InitParameters]
	{
		auto task = new FAsyncTask<CookAkSoundDataTask>(InitParameters);
		task->StartSynchronousTask();
		task->EnsureCompletion();
		AsyncTask(ENamedThreads::GameThread, [task]
		{
			delete task;
		});

	});
}

void CookAkSoundDataTask::DoWork()
{
	//Disable autosave during sound data generation
	UEditorLoadingSavingSettings* LoadingSavingSettings = GetMutableDefault<UEditorLoadingSavingSettings>();
	const bool bOldAutoSaveState = LoadingSavingSettings->bAutoSaveEnable;
	LoadingSavingSettings->bAutoSaveEnable = false;
	_dataSource->DoWork();
	LoadingSavingSettings->bAutoSaveEnable = bOldAutoSaveState;
}
