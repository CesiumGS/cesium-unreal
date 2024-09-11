// Copyright 2020-2024 CesiumGS, Inc. and Contributors

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

public class CesiumEditor : ModuleRules
{
    public CesiumEditor(ReadOnlyTargetRules Target) : base(Target)
    {
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

        string platform;
        string libSearchPattern;
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            platform = "Windows-AMD64-";
            libSearchPattern = "*.lib";
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            platform = "Darwin-universal-";
            libSearchPattern = "lib*.a";
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            platform = "Android-aarch64-";
            libSearchPattern = "lib*.a";
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            platform = "Linux-x86_64-";
            libSearchPattern = "lib*.a";
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            platform = "iOS-ARM64-";
            libSearchPattern = "lib*.a";
        }
        else
        {
            throw new InvalidOperationException("Cesium for Unreal does not support this platform.");
        }

        string libPathBase = Path.Combine(ModuleDirectory, "../ThirdParty/lib/" + platform);
        string libPathDebug = libPathBase + "Debug";
        string libPathRelease = libPathBase + "Release";

        bool useDebug = false;
        if (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            if (Directory.Exists(libPathDebug))
            {
                useDebug = true;
            }
        }

        string libPath = useDebug ? libPathDebug : libPathRelease;

        string[] allLibs = Directory.Exists(libPath) ? Directory.GetFiles(libPath, libSearchPattern) : new string[0];

        PublicAdditionalLibraries.AddRange(allLibs);

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "UnrealEd",
                "CesiumRuntime",
                "OpenSSL"
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
                "DeveloperSettings",
                "EditorStyle"
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

        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_2;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
    }
}
