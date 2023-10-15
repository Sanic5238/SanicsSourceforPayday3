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

#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "WwiseItemType.h"
#include "Misc/DateTime.h"

class WorkUnitXmlVisitor;
class FXmlFile;
class FXmlNode;
struct FScopedSlowTask;

class WwiseWorkUnitParser
{
public:
	void SetVisitor(WorkUnitXmlVisitor* VisitorInstance) { Visitor = VisitorInstance; }

	bool Parse();

	bool ForceParse();
	void ParsePhysicalFolders();

	struct WorkUnitInfo
	{
		bool successfullyParsed;
		bool isStandalone;
		FString wwuName;
		FString wwuPath;
		FGuid wwuGuid;
		FGuid parentWorkUnitGuid;
		FGuid parentGuid;
	};

private:
	void parseFolders(const FString& FolderName, EWwiseItemType::Type ItemType);
	void parseWorkUnitFile(const WorkUnitInfo& wwuInfo, const FString& RelativePath, EWwiseItemType::Type ItemType, FScopedSlowTask* SlowTask =nullptr);
	void preParseWorkUnits(const TArray<FString>& WwusToProcess, EWwiseItemType::Type ItemType);
	WorkUnitInfo peekWorkUnit(const FString& WwuFilePath, EWwiseItemType::Type ItemType);

	bool parseWorkUnitXml( const FString& WorkUnitPath, const FString& RelativePath, EWwiseItemType::Type ItemType);
	void parseWorkUnitChildren(const FXmlNode* NodeToParse, const FString& WorkUnitPath, const FString& RelativePath, EWwiseItemType::Type ItemType, const FGuid& parentId);
	void recurse(const FXmlNode* CurrentNode, const FString& WorkUnitPath, const FString& CurrentPath, EWwiseItemType::Type ItemType, const FGuid& parentId);
	FString getRelativePath(const WorkUnitInfo& info, EWwiseItemType::Type ItemType);

	TMap<EWwiseItemType::Type, TMap<FString, FDateTime>> wwuLastPopulateTime;
	TMap<FGuid, WorkUnitInfo> nestedWwusToParse;
	TMap<FGuid, WorkUnitInfo> standaloneWwusToParse;
	TArray<FGuid> parsedWwus;
	TArray<FString> unparseableWwus;
	FString projectRootFolder;
	WorkUnitXmlVisitor* Visitor = nullptr;
};