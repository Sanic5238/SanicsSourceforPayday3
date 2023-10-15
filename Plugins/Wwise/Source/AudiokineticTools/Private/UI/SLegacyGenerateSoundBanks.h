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
	SLegacyGenerateSoundBanks.h
------------------------------------------------------------------------------------*/
#pragma once

/*------------------------------------------------------------------------------------
	SLegacyGenerateSoundBanks
------------------------------------------------------------------------------------*/
#include "AkAudioBank.h"
#include "AkAudioEvent.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

class SLegacyGenerateSoundBanks : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SLegacyGenerateSoundBanks )
	{}
	SLATE_END_ARGS( )

	SLegacyGenerateSoundBanks(void);

	AUDIOKINETICTOOLS_API void Construct(const FArguments& InArgs, TArray<TWeakObjectPtr<UAkAudioBank>>* in_pSoundBanks);
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent ) override;

	/** override the base method to allow for keyboard focus */
	virtual bool SupportsKeyboardFocus() const
	{
		return true;
	}

	bool ShouldDisplayWindow() { return PlatformNames.Num() != 0; }

    /* Set whether the Wwise project should be saved before the soundbanks are generated. */
    void SetShouldSaveWwiseProject(bool in_bShouldSaveBeforeGeneration);

private:
	void PopulateList();

private:
	FReply OnGenerateButtonClicked();
	TSharedRef<ITableRow> MakeBankListItemWidget(TSharedPtr<FString> Bank, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> MakePlatformListItemWidget(TSharedPtr<FString> Platform, const TSharedRef<STableViewBase>& OwnerTable);
	bool FetchAttenuationInfo(const TMap<FString, TSet<UAkAudioEvent*> >& BankToEventSet);

private:
	TSharedPtr< SListView < TSharedPtr<FString> > > BankList;
	TSharedPtr< SListView < TSharedPtr<FString> > > PlatformList;

	TArray< TSharedPtr<FString> > Banks;
	TArray< TSharedPtr<FString> > PlatformNames;
    /* Determines whether the Wwise project is saved (via WAAPI) before the soundbanks are generated. */
    bool m_bShouldSaveWwiseProject = false;

	bool EnsureSoundBankPathIsInPackagingSettings();

};