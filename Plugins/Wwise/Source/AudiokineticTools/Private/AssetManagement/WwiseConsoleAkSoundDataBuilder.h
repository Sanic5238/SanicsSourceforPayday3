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

#include "AkSoundDataBuilder.h"

class FDirectoryWatcherModule;

class WwiseConsoleAkSoundDataBuilder : public AkSoundDataBuilder
{
public:
	WwiseConsoleAkSoundDataBuilder(const InitParameters& InitParameter);
	~WwiseConsoleAkSoundDataBuilder();

	void Init() override;
	void DoWork() override;

	void SetOverrideWwiseConsolePath(const FString& value) { overrideWwiseConsolePath = value; }

private:
	void OnDirectoryWatcher(const TArray<struct FFileChangeData>& ChangedFiles);

	bool runWwiseConsole();
	void ProcessFile(const FString& fullPath, const FString& generatedSoundBanksFolder);

	bool readBankData(UAkAssetData* AssetData, const FString& BankFile, IPlatformFile& FileManager, FCriticalSection* DataLock);

	TSharedPtr<FJsonObject> readJsonFile(const FString& JsonFileName);

	template<typename MainAsset, typename PlatformAsset>
	bool ReadBankDefinitionFile(MainAsset* mainAsset, PlatformAsset* platformAsset, const FString& platform, const FString& language, const FString& jsonFile);

	bool readPluginInfo(UAkInitBank* InitBank, const FString& Platform, const FString& PluginInfoFileName);

	void prepareRebuild(const FString& BankName, const FString& GeneratedSoundBanksPath);

private:
	FString watchDirectory;
	FDelegateHandle directoryWatcherDelegateHandle;
	FDirectoryWatcherModule* directoryWatcherModule = nullptr;

	FGraphEventArray AllWatcherTasks;
	TSet<FString> ProcessedPaths;

	FString overrideWwiseConsolePath;
};
