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
	SClearSoundData.h
------------------------------------------------------------------------------------*/
#pragma once

#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"

class SCheckBox;

class SClearSoundData : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SClearSoundData )
	{}
	SLATE_END_ARGS( )

	SClearSoundData(void);

	AUDIOKINETICTOOLS_API void Construct(const FArguments& InArgs);
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent ) override;

	/** override the base method to allow for keyboard focus */
	virtual bool SupportsKeyboardFocus() const
	{
		return true;
	}
		

	bool GetDeleteLocalizedEnabled() const
	{
		return (ClearAssetData && ClearAssetData->IsChecked());
	}

private:
	FReply OnClearButtonClicked();

private:
	TSharedPtr<SCheckBox> ClearAssetData;
	TSharedPtr<SCheckBox> DeleteLocalizedPlatformData;
	TSharedPtr<SCheckBox> ClearSoundBankInfoCache;
	TSharedPtr<SCheckBox> ClearMediaCache;
	TSharedPtr<SCheckBox> ClearExternalSource;
	TSharedPtr<SCheckBox> ClearOrphanMedia;
	TSharedPtr<SCheckBox> ClearOrphanLocalizedAssetData;
};