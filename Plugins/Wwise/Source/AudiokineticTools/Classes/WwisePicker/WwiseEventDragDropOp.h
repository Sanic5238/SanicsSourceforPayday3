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


/*------------------------------------------------------------------------------------
	WwiseEventDragDropOp.h
------------------------------------------------------------------------------------*/
#pragma once

#include "WaapiPicker/WwiseTreeItem.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "ContentBrowserDelegates.h"

class FWwiseEventDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FWwiseEventDragDropOp, FDecoratedDragDropOp)

	const FSlateBrush* Icon;
	
	static TSharedRef<FWwiseEventDragDropOp> New(TArray<TSharedPtr<FWwiseTreeItem>> InAssets);
	~FWwiseEventDragDropOp();
	virtual FCursorReply OnCursorQuery() override;

	void SetCanDrop(const bool CanDrop);
	bool OnAssetViewDrop(const FAssetViewDragAndDropExtender::FPayload& Payload);
	bool OnAssetViewDragOver(const FAssetViewDragAndDropExtender::FPayload& Payload);
	bool OnAssetViewDragLeave(const FAssetViewDragAndDropExtender::FPayload& Payload);
	static UObject* RecurseCreateAssets(const TSharedPtr<FWwiseTreeItem>& Asset, const FString& PackagePath);

	const TArray<TSharedPtr<FWwiseTreeItem>>& GetWiseItems() const;

	void SetTooltipText();
	FText GetTooltipText() const;
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
	FAssetViewDragAndDropExtender* pExtender;

private:	
	
	/** Data for the asset this item represents */
	TArray<TSharedPtr<FWwiseTreeItem>> WwiseAssets;

	bool CanDrop;
};