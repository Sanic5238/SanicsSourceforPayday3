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

public class AudiokineticTools : ModuleRules
{
#if WITH_FORWARDED_MODULE_RULES_CTOR
    public AudiokineticTools(ReadOnlyTargetRules Target) : base(Target)
#else
    public AudiokineticTools(TargetInfo Target)
#endif
    {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add("AudiokineticTools/Private");
        PrivateIncludePaths.Add("AkAudio/Classes/GTE");
        PrivateIncludePathModuleNames.AddRange(
            new string[]
            {
                "TargetPlatform",
                "MainFrame",
				"MovieSceneTools"
            });

        PublicIncludePathModuleNames.AddRange(
            new string[] 
            { 
                "AssetTools",
                "ContentBrowser",
#if UE_4_24_OR_LATER
                "ToolMenus"
#endif
            });

        PublicDependencyModuleNames.AddRange(
            new string[] 
            { 
                "AkAudio",
                "Core",
                "InputCore",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "EditorStyle",
				"Json",
				"XmlParser",
				"WorkspaceMenuStructure",
				"DirectoryWatcher",
                "Projects",
				"Sequencer",
                "PropertyEditor",
                "SharedSettingsWidgets",
                "ContentBrowser",
#if UE_5_0_OR_LATER
                "DeveloperToolSettings",
#endif

#if UE_4_26_OR_LATER
                "ContentBrowserData",
#endif
#if UE_4_24_OR_LATER
                "ToolMenus"
#endif
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
			{
				"MovieScene",
				"DesktopPlatform",
				"MovieSceneTools",
				"MovieSceneTracks",
				"RenderCore",
				"SourceControl",
                "LevelEditor",
#if UE_5_0_OR_LATER
                "EditorFramework"
#endif
            });

#if UE_4_26_OR_LATER
        PrivateDependencyModuleNames.Add("RHI");
#endif
    }
}
