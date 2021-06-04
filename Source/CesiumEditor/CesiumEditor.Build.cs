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

        string[] libs = new string[]
        {
            "CesiumIonClient",
            "csprng"
        };

        string platform;
        if (Target.Platform == UnrealTargetPlatform.Win64) {
            platform = "Windows-x64";
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac) {
            platform = "Darwin-x64";
        }
        else if(Target.Platform == UnrealTargetPlatform.Android) {
            platform = "Android-xaarch64";
        }
        else if(Target.Platform == UnrealTargetPlatform.Linux) {
            platform = "Linux-x64";
        }
        else {
            platform = "Unknown"; 
        }

        string libPath = Path.Combine(ModuleDirectory, "../ThirdParty/lib/" + platform);
        var libFiles = Directory.GetFiles(libPath).Where(path => libs.Any(lib => path.Contains(lib)));
        PublicAdditionalLibraries.AddRange(libFiles);

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
