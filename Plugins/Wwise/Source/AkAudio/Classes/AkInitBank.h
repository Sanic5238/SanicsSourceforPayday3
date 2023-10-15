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

#include "AkAssetBase.h"
#include "AkInitBank.generated.h"


DECLARE_DELEGATE_OneParam(FOnInitBankChanged, UAkInitBank*)

USTRUCT()
struct AKAUDIO_API FAkPluginInfo
{
	GENERATED_BODY()

public:
	FAkPluginInfo() = default;

	FAkPluginInfo(const FString& InName, uint32 InPluginID, const FString& InDLL)
	: Name(InName)
	, PluginID(InPluginID)
	, DLL(InDLL)
	{
	}

	UPROPERTY(VisibleAnywhere, Category = "AkInitBank")
	FString Name;

	UPROPERTY(VisibleAnywhere, Category = "AkInitBank")
	uint32 PluginID = 0;

	UPROPERTY(VisibleAnywhere, Category = "AkInitBank")
	FString DLL;
};

UCLASS()
class AKAUDIO_API UAkInitBankAssetData : public UAkAssetDataWithMedia
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "AkInitBank")
	TArray<FAkPluginInfo> PluginInfos;
};

UCLASS()
class AKAUDIO_API UAkInitBank : public UAkAssetBase
{
	GENERATED_BODY()
	UAkInitBank();
public:
	UPROPERTY(VisibleAnywhere, Category = "AkInitBank")
	TArray<FString> AvailableAudioCultures;

	UPROPERTY(VisibleAnywhere, Category = "AkInitBank")
	FString DefaultLanguage;

	virtual void LoadBank() override;

#if WITH_EDITOR
	void Reset(TArray<FAssetData>& InOutAssetsToDelete) override;
	static FOnInitBankChanged OnInitBankChanged;
#endif

protected:
	UAkAssetData* CreateAssetData(UObject* parent) const override;
};
