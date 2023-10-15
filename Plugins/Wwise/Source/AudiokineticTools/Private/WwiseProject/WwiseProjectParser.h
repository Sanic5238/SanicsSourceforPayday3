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
#include "WwiseItemType.h"
#include "FastXml.h"

class WorkUnitXmlVisitor;

class WwiseProjectParser : public IFastXmlCallback
{
public:
	WwiseProjectParser(WorkUnitXmlVisitor* vis);
	virtual ~WwiseProjectParser() {};

private:
	FString currentPhysicalPath;
	FString workUnitName;
	FGuid currentId;
	bool inPhysicalFolder;
	bool inWorkUnit;
	WorkUnitXmlVisitor* visitor;

	bool ProcessXmlDeclaration(const TCHAR* ElementData, int32 XmlFileLineNumber) override;

	bool ProcessElement(const TCHAR* ElementName, const TCHAR* ElementData, int32 XmlFileLineNumber) override;

	bool ProcessAttribute(const TCHAR* AttributeName, const TCHAR* AttributeValue) override;

	bool ProcessClose(const TCHAR* ElementName) override;

	bool ProcessComment(const TCHAR* Comment) override;
};