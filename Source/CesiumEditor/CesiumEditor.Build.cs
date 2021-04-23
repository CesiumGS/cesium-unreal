// Copyright 2020-2021 CesiumGS, Inc. and Contributors

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

public class CesiumEditor : ModuleRules
{
    public CesiumEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
        );


        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "../ThirdParty/include")
            }
        );

        string libPrefix;
        string libPostfix;
        string platform;
        if (Target.Platform == UnrealTargetPlatform.Win64) {
            platform = "Windows-x64";
            libPostfix = ".lib";
            libPrefix = "";
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac) {
            platform = "Darwin-x64";
            libPostfix = ".a";
            libPrefix = "lib";
        }
        else if(Target.Platform == UnrealTargetPlatform.Linux) {
            platform = "Linux-x64";
            libPostfix = ".a";
            libPrefix = "lib";
        }
        else {
            platform = "Unknown";
            libPostfix = ".Unknown";
            libPrefix = "Unknown";
        }

        string libPath = Path.Combine(ModuleDirectory, "../ThirdParty/lib/" + platform);

        string releasePostfix = "";
        string debugPostfix = "d";

        bool preferDebug = (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame);
        string postfix = preferDebug ? debugPostfix : releasePostfix;

        string[] libs = new string[]
        {
            "CesiumIonClient",
            "csprng"
        };

        if (preferDebug)
        {
            // We prefer Debug, but might still use Release if that's all that's available.
            foreach (string lib in libs)
            {
                string debugPath = Path.Combine(libPath, libPrefix + lib + debugPostfix + libPostfix);
                if (!File.Exists(debugPath))
                {
                    Console.WriteLine("Using release build of cesium-native because a debug build is not available.");
                    preferDebug = false;
                    postfix = releasePostfix;
                    break;
                }
            }
        }

        PublicAdditionalLibraries.AddRange(libs.Select(lib => Path.Combine(libPath, libPrefix + lib + postfix + libPostfix)));

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "UnrealEd",
                "CesiumRuntime"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "ApplicationCore",
                "Slate",
                "SlateCore",
                "MeshDescription",
                "StaticMeshDescription",
                "HTTP",
                "MikkTSpace",
                "Chaos",
                "Projects",
                "InputCore",
                "PropertyEditor",
                "DeveloperSettings"
				// ... add private dependencies that you statically link with here ...	
			}
        );

        PublicDefinitions.AddRange(
            new string[]
            {
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "Private/PCH.h";
        CppStandard = CppStandardVersion.Cpp17;
    }
}
