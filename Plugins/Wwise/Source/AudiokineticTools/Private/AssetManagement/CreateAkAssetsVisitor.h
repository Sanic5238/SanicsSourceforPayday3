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

#include "WwiseProject/WorkUnitXmlVisitor.h"

#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Misc/Guid.h"

struct FAssetData;
class UPackage;

class CreateAkAssetsVisitor : public WorkUnitXmlVisitor
{
public:
	void OnBeginParse() override;

	void EnterWorkUnit(const WwiseWorkUnitParser::WorkUnitInfo& wwuInfo, const FString& RelativePath, EWwiseItemType::Type ItemType) override;

	void EnterEvent(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterAcousticTexture(const FGuid& Id, const class FXmlNode* CurrentNode, const FString& Name, const FString& RelativePath) override;

	void EnterAuxBus(const FGuid& Id, const FString& Name, const FString& RelativePath) override;
	
	void EnterStateGroup(const FGuid& Id, const FString& Name, const FString& RelativePath) override;
	void EnterState(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterSwitchGroup(const FGuid& Id, const FString& Name, const FString& RelativePath) override;
	void EnterSwitch(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterGameParameter(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterTrigger(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterFolderOrBus(const FGuid& Id, const FString& Name, const FString& RelativePath,  EWwiseItemType::Type ItemType) override;
	
	void End() override;

	void RegisterError(const FString& xmlFilePath, const FString& errorMessage) override;
	FString FindRelativePath(const FString& WorkUnitPath, const FGuid& id, EWwiseItemType::Type ItemType) override;

protected:
	virtual void collectExtraAssetsToDelete(TArray<FAssetData>& assetToDelete) {}

private:
	template<typename AssetType>
	void createAsset(const FGuid& Id, const FString& Name, const FString& AssetName, const FString& RelativePath, const FGuid& GroupId = FGuid());
	void createFolder(const FGuid& Id, const FString& RelativePath, const FString& Name, const  EWwiseItemType::Type ItemType);

protected:
	TArray<UPackage*> packagesToSave;

private:
	FString currentFolderPath;
	TSet<FGuid> foundAssets;

	FString currentStateGroupName;
	FGuid currentStateGroupId;

	FString currentSwitchGroupName;
	FGuid currentSwitchGroupId;
	bool doAssetCleanup = true;
};

