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

using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;

public class AkUEPlatform_HoloLens : AkUEPlatform
{
	public AkUEPlatform_HoloLens(ReadOnlyTargetRules in_TargetRules, string in_ThirdPartyFolder) : base(in_TargetRules, in_ThirdPartyFolder) {}

#if !UE_4_24_OR_LATER
	public override string SanitizeLibName(string in_libName)
	{
		return in_libName + ".lib";
	}
#endif

	public override string GetLibraryFullPath(string LibName, string LibPath)
	{
		return Path.Combine(LibPath, LibName + ".lib");
	}

	public override bool SupportsAkAutobahn { get { return false; } }

	public override bool SupportsCommunication { get { return true; } }

	public override bool SupportsDeviceMemory { get { return false; } }

	public override string AkPlatformLibDir { get { return "UWP_ARM64_" + GetVisualStudioVersion(); } }

	public override string DynamicLibExtension { get { return "dll"; } }

	public override List<string> GetAdditionalWwiseLibs()
	{
		return new List<string>();
	}
	
	public override List<string> GetPublicSystemLibraries()
	{
		return new List<string>();
	}

	public override List<string> GetPublicDelayLoadDLLs()
	{
		return new List<string>();
	}

	public override List<string> GetPublicDefinitions()
	{
		return new List<string>
		{
			"AK_HOLOLENS_VS_VERSION=\"" + GetVisualStudioVersion() + "\""
		};
	}

	public override Tuple<string, string> GetAdditionalPropertyForReceipt(string ModuleDirectory)
	{
		return null;
	}

	public override List<string> GetPublicFrameworks()
	{
		return new List<string>();
	}

	private string GetVisualStudioVersion()
	{
#if UE_4_25_OR_LATER
		return "vc160";
#else
		return "vc150";
#endif
	}
}
