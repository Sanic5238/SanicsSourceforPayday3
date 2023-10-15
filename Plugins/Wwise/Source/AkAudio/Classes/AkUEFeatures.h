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

// Defines which features of the Wwise-Unreal integration are supported in which version of UE.

#pragma once

#include "AK/AkWwiseSDKVersion.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "Misc/RedirectCollector.h"
#endif

#define UE_4_21_OR_LATER ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 21) || (ENGINE_MAJOR_VERSION >= 5))
#define UE_4_22_OR_LATER ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 22) || (ENGINE_MAJOR_VERSION >= 5))
#define UE_4_23_OR_LATER ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 23) || (ENGINE_MAJOR_VERSION >= 5))
#define UE_4_24_OR_LATER ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 24) || (ENGINE_MAJOR_VERSION >= 5))
#define UE_4_25_OR_LATER ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 25) || (ENGINE_MAJOR_VERSION >= 5))
#define UE_4_26_OR_LATER ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26) || (ENGINE_MAJOR_VERSION >= 5))
#define UE_4_27_OR_LATER ((ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27) || (ENGINE_MAJOR_VERSION >= 5))
#define UE_5_0_OR_LATER   (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 0)
#define UE_5_1_OR_LATER   (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
#define UE_5_2_OR_LATER   (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)

#if UE_5_2_OR_LATER
#include "AssetRegistry/AssetData.h"
#endif

#if UE_4_22_OR_LATER
#define AK_DEPRECATED UE_DEPRECATED
#else
#define AK_DEPRECATED DEPRECATED
#endif

// PhysX (deprecated in UE5.0) and Chaos (mandatory in UE5.1) selection
#if UE_5_1_OR_LATER
#define AK_USE_CHAOS 1
#define AK_USE_PHYSX 0
#else
#if defined(PHYSICS_INTERFACE_PHYSX) && PHYSICS_INTERFACE_PHYSX
#define AK_USE_PHYSX 1
#else
#define AK_USE_PHYSX 0
#endif
#if defined(WITH_CHAOS) && WITH_CHAOS
#define AK_USE_CHAOS 1
#else
#define AK_USE_CHAOS 0
#endif
#endif

// Styling naming changed between UE4 and UE5.
#if WITH_EDITOR
#if UE_5_0_OR_LATER
#include "Styling/AppStyle.h"
using FAkAppStyle = FAppStyle;
#else
#include "EditorStyleSet.h"
using FAkAppStyle = FEditorStyle;
#endif
#endif

// Audio Lab / Demo Features

#if (AK_WWISESDK_VERSION_MAJOR >= 2021 && AK_WWISESDK_VERSION_MINOR >=1 && AK_WWISESDK_VERSION_SUBMINOR >=3)
#define AK_OUTPUT_DEVICE_METERING_ENABLED 
#endif

/******** Bulk data defines */
#include "Serialization/BulkData.h"
// Bulk data IO request type changes depending on build settings.
#if UE_5_1_OR_LATER
#define AK_RUNTIME_BULKDATA USE_RUNTIME_BULKDATA
using BulkDataIORequest = IBulkDataIORequest;
#elif !WITH_EDITOR
#if defined(USE_NEW_BULKDATA) && USE_NEW_BULKDATA
#define AK_RUNTIME_BULKDATA 1
using BulkDataIORequest = IBulkDataIORequest;
#else
#define AK_RUNTIME_BULKDATA 0
using BulkDataIORequest = FBulkDataIORequest;
#endif
#else
#define AK_RUNTIME_BULKDATA 0
#if UE_4_25_OR_LATER
using BulkDataIORequest = IBulkDataIORequest;
#else
using BulkDataIORequest = FBulkDataIORequest;
#endif
#endif

// UE 4.25 changed the callback type when a bulk data IO request finishes
#if UE_4_25_OR_LATER
using BulkDataRequestCompletedCallback = FBulkDataIORequestCallBack;
using ReadRequestArgumentType = IBulkDataIORequest;
#else
using BulkDataRequestCompletedCallback = FAsyncFileCallBack;
using ReadRequestArgumentType = IAsyncReadRequest;
#endif

// UE 5.0 typedefs
#include "Containers/Ticker.h"
#if UE_5_0_OR_LATER
using FUnrealFloatVector = FVector3f;
using FUnrealFloatVector2D = FVector2f;
using FUnrealFloatPlane = FPlane4f;
using FTickerDelegateHandle = FTSTicker::FDelegateHandle;
using FCoreTickerType = FTSTicker;
#else
using FUnrealFloatVector = FVector;
using FUnrealFloatVector2D = FVector2D;
using FCoreTickerType = FTicker;
using FUnrealFloatPlane = FPlane;
using FTickerDelegateHandle = FDelegateHandle;
#endif

// Helper functions for functions deprecated in UE 5.1
inline UClass* AkUEFindObject(const FString& ClassName, bool bExactClass = false)
{
#if UE_5_1_OR_LATER
	return UClass::TryFindTypeSlow<UClass>(ClassName, bExactClass ? EFindFirstObjectOptions::ExactClass : EFindFirstObjectOptions::None);
#else 
	return FindObject<UClass>(ANY_PACKAGE, *ClassName, bExactClass);
#endif
}

inline void AkUEGetAssetsByClass(const UClass* StaticClass, TArray<FAssetData>& OutAssets, bool bSearchSubClasses = false)
{
	const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	auto& AssetRegistry = AssetRegistryModule.Get();
#if UE_5_1_OR_LATER
	AssetRegistry.GetAssetsByClass(StaticClass->GetClassPathName(), OutAssets, bSearchSubClasses);
#else
	AssetRegistry.GetAssetsByClass(StaticClass->GetFName(), OutAssets, bSearchSubClasses);
#endif
}

inline FAssetData AkUEGetAssetByObjectPath(const FSoftObjectPath& SoftObjectPath)
{
	const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	auto& AssetRegistry = AssetRegistryModule.Get();
#if UE_5_1_OR_LATER
	return AssetRegistry.GetAssetByObjectPath(SoftObjectPath);
#else
	return AssetRegistry.GetAssetByObjectPath(SoftObjectPath.GetAssetPathName());
#endif
}

inline FString AkUEGetObjectPathString(const FAssetData& AssetData)
{
#if UE_5_1_OR_LATER
	return AssetData.GetObjectPathString();
#else
	return AssetData.ObjectPath.ToString();
#endif
}

inline FString AkUEGetObjectPathString(const FAssetData* AssetData)
{
#if UE_5_1_OR_LATER
	return AssetData->GetObjectPathString();
#else
	return AssetData->ObjectPath.ToString();
#endif
}

inline FSoftObjectPath AkUEGetObjectPath(const FAssetData& AssetData)
{
#if UE_5_1_OR_LATER
	return AssetData.GetSoftObjectPath();
#else
	return AssetData.ToSoftObjectPath();
#endif
}

inline bool DoesAssetMatchClass(const FAssetData& AssetData, const UClass* StaticClass)
{
#if UE_5_1_OR_LATER
	return AssetData.AssetClassPath == StaticClass->GetClassPathName();
#else
	return AssetData.AssetClass == StaticClass->GetFName();
#endif
}

#if WITH_EDITOR
inline void AkUEAddAssetPathRedirection(const FString& OldPath, const FString& NewPath)
{
#if UE_5_1_OR_LATER
	GRedirectCollector.AddAssetPathRedirection(FSoftObjectPath(OldPath), FSoftObjectPath(NewPath));
#else
	GRedirectCollector.AddAssetPathRedirection(FName(*OldPath), FName(*NewPath));
#endif
}
#endif