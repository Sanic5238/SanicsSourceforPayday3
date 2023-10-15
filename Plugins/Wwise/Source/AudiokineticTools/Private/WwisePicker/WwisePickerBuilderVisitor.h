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
#include "Misc/TextFilter.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"
#include "WaapiPicker/WwiseTreeItem.h"

using StringFilter = TTextFilter<const FString&>;

class WwisePickerBuilderVisitor : public WorkUnitXmlVisitor
{
public:
	WwisePickerBuilderVisitor();

	void ForceInit() override;
	void OnBeginPhysicalFolderParse() override;

	void EnterWorkUnit(const WwiseWorkUnitParser::WorkUnitInfo& wwuInfo, const FString& RelativePath, EWwiseItemType::Type ItemType) override;
	void ExitWorkUnit(bool IsStandaloneWorkUnit) override; 

	void EnterEvent(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterAcousticTexture(const FGuid& Id, const class FXmlNode* CurrentNode, const FString& Name, const FString& RelativePath) override;

	void EnterAuxBus(const FGuid& Id, const FString& Name, const FString& RelativePath) override;
	void ExitAuxBus() override;

	void EnterFolderOrBus(const FGuid& Id, const FString& Name, const FString& RelativePath, EWwiseItemType::Type ItemType) override;
	void ExitFolderOrBus() override;
	void EnterStateGroup(const FGuid& Id, const FString& Name, const FString& RelativePath) override;
	void ExitStateGroup() override;

	void EnterState(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterSwitchGroup(const FGuid& Id, const FString& Name, const FString& RelativePath) override;
	void ExitSwitchGroup() override;

	void EnterSwitch(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterGameParameter(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void EnterTrigger(const FGuid& Id, const FString& Name, const FString& RelativePath) override;

	void ExitChildrenList() override;

	void EndPhysicalFolderParse() override;

	void RemoveWorkUnit(const FString& WorkUnitPath) override;
	FString FindRelativePath(const FString& WorkUnitPath, const FGuid& id, EWwiseItemType::Type ItemType) override;

	TSharedPtr<FWwiseTreeItem> GetTree(TSharedPtr<StringFilter> SearchBoxFilter, TSharedPtr<FWwiseTreeItem> CurrentTreeRootItem, EWwiseItemType::Type ItemType);
	void CopyTree(TSharedPtr<FWwiseTreeItem> SourceTreeItem, TSharedPtr<FWwiseTreeItem> DestTreeItem);
	void FilterTree(TSharedPtr<FWwiseTreeItem> TreeItem, TSharedPtr<StringFilter> SearchBoxFilter);


private:
	void createChildTreeItem(const FString& Name, const FString& RelativePath, EWwiseItemType::Type ItemType, const FGuid& Id);
	void createPhysicalFolderItem(const FString& Name, const FString& RelativePath, const FGuid& Id);
	void createParentTreeItem(const FString& Name, const FString& RelativePath, EWwiseItemType::Type ItemType, const FGuid& Id);
	void removeTreeItem(FGuid Id);
	void removeTreeItemRecursive(TSharedPtr<FWwiseTreeItem> treeItem);
	TSet<FGuid> getKnownPhysicalFolders();
	void exitParent();



private:
	FString OldFilterText;
	FString projectRootFolder;

	TMap<FString, FGuid> WwuPathToGuid;
	TMap<FString, FGuid> PhysicalPathToGuid;

	TMap<EWwiseItemType::Type, TSharedPtr<FWwiseTreeItem>> TypeTreeRoots;
	TMap<FGuid, TSharedPtr<FWwiseTreeItem>> treeItems;
	TArray<TWeakPtr<FWwiseTreeItem>> rootTreeItemStack;

	TSet<FGuid> foundPhysicalFolders;
};
