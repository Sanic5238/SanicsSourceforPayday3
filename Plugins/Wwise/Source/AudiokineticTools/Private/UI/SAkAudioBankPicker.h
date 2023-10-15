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

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SCompoundWidget.h"

#include "AssetRegistry/AssetData.h"

class SWindow;

class SAkAudioBankPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAkAudioBankPicker)
	{}
		SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_END_ARGS()

	SAkAudioBankPicker(void);

	AUDIOKINETICTOOLS_API void Construct(const FArguments& InArgs);

	FAssetData SelectedAkEventGroup;

private:
	void OnCreateNewAssetSelected();
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);
	void OnAssetEnterPressed(const TArray<FAssetData>& AssetData);
	bool CanSelect() const;

	FReply CloseWindow();
	FReply OnCancel();

private:
	TWeakPtr<SWindow> WidgetWindow;
};