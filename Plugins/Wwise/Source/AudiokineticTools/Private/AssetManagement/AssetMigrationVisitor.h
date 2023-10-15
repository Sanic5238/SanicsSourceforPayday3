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

#include "CreateAkAssetsVisitor.h"
#include "Containers/Map.h"
#include "Containers/Array.h"

class AssetMigrationVisitor : public CreateAkAssetsVisitor
{
	using Super = CreateAkAssetsVisitor;
public:
	void OnBeginParse() override;

	void EnterEvent(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterAuxBus(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterAcousticTexture(const FGuid& Id, const class FXmlNode* CurrentNode, const FString& Name, const FString& RelativePath) override;

	void End() override;

protected:
	void collectExtraAssetsToDelete(TArray<FAssetData>& assetToDelete) override;

	template<typename AssetType>
	void migrateAssets(const FGuid& Id, const FString& Name, TMap<FString, TArray<UObject*>>& duplicatedAssets);

private:
	TMap<FString, TArray<UObject*>> duplicatedEvents;
	TMap<FString, TArray<UObject*>> duplicatedAuxBus;
	TMap<FString, TArray<UObject*>> duplicatedAcousticTextures;

	bool AkConsolidateObjects(UObject* ObjectToConsolidateTo, TArray<UObject*>& ObjectsToConsolidate);

};