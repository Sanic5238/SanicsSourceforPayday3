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

#include "UObject/Object.h"
#include "AkInclude.h"
#include "AkAudioType.generated.h"

UCLASS()
class AKAUDIO_API UAkAudioType : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "AkAudioType")
	FGuid ID;
#endif

#if WITH_EDITOR
	bool bChangedDuringReset = false;
	virtual void Reset(TArray<FAssetData>& InOutAssetsToDelete);
#endif

	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category="AkAudioType")
	uint32 ShortID = 0;

	UPROPERTY(EditAnywhere, Category = "AkAudioType")
	TArray<UObject*> UserData;

	void PostLoad() override;

	virtual void LoadBank();

	void ValidateShortId(bool bMarkAsDirty);
	bool ShortIdMatchesName(AkUInt32& OutIdFromName);
	void SetShortId(const AkUInt32& IdFromName, bool bMarkAsDirty);
	
	void MarkDirtyInGameThread();

	virtual bool CanBeClusterRoot() const override { return false; }
	virtual bool CanBeInCluster() const override { return false; }

public:
	template<typename T>
	T* GetUserData()
	{
		for (auto Entry : UserData)
		{
			if (Entry && Entry->GetClass()->IsChildOf(T::StaticClass()))
			{
				return Entry;
			}
		}

		return nullptr;
	}
};
