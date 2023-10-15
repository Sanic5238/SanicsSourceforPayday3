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

#include "AkAudioType.h"
#include "AkGroupValue.generated.h"

class UAkMediaAsset;

UCLASS()
class AKAUDIO_API UAkGroupValue : public UAkAudioType
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "AkGroupValue")
	FGuid GroupID;
#endif
	
	UPROPERTY(VisibleAnywhere, Category = "AkGroupValue")
	TArray<TSoftObjectPtr<UAkMediaAsset>> MediaDependencies;

	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "AkGroupValue")
	uint32 GroupShortID = 0;

	void SignalMediaDependencyIsLoaded(UAkMediaAsset* Media);

public:
	void PostLoad() override;

	void BeginDestroy() override;
	void Serialize(FArchive& Ar) override;
	bool ExternalReadCallback(double RemainingTime, TSoftObjectPtr<UAkMediaAsset> Media);
	bool IsReadyForAsyncPostLoad() const override;

#if WITH_EDITOR
	void Reset(TArray<FAssetData>& InOutAssetsToDelete) override;
#endif

private:
	TArray<TWeakObjectPtr<UAkMediaAsset>> LoadedMediaDependencies;
};
