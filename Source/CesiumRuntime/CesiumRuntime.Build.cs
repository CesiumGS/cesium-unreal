// Copyright 2020-2021 CesiumGS, Inc. and Contributors

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

public class CesiumRuntime : ModuleRules
{
    public CesiumRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "../ThirdParty/include")
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
        );

        string[] libs = new string[]
        {
            "async++",
            "Cesium3DTiles",
            "CesiumAsync",
            "CesiumGeometry",
            "CesiumGeospatial",
            "CesiumGltfReader",
            "CesiumGltf",
            "CesiumJsonReader",
            "CesiumUtility",
            "draco",
            //"MikkTSpace",
            "modp_b64",
            "spdlog",
            "sqlite3",
            "tinyxml2",
            "uriparser",
            "fmt",
        };

        // Use our own copy of MikkTSpace on Android.
        if (Target.Platform == UnrealTargetPlatform.Android) {
            libs = libs.Concat(new string[] { "MikkTSpace" }).ToArray();
            PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/include/mikktspace"));
        }

        string platform;
        if (Target.Platform == UnrealTargetPlatform.Win64) {
            platform = "Windows-x64";
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac) {
            platform = "Darwin-x64";
        }
        else if (Target.Platform == UnrealTargetPlatform.Android) {
            platform = "Android-xaarch64";
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux) {
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
                // ... add other public dependencies that you statically link with here ...
            }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "RHI",
                "CoreUObject",
                "Engine",
                "MeshDescription",
                "StaticMeshDescription",
                "HTTP",
                "LevelSequence",
                "Projects"
            }
        );
        
        // Use UE's MikkTSpace on non-Android
        if (Target.Platform != UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("MikkTSpace");
        }


        PublicDefinitions.AddRange(
            new string[]
            {
                "SPDLOG_FMT_EXTERNAL",
                "SPDLOG_COMPILED_LIB",
                "LIBASYNC_STATIC"
            }
        );

        if (Target.bCompilePhysX && !Target.bUseChaos)
        {
            PrivateDependencyModuleNames.Add("PhysXCooking");
            PrivateDependencyModuleNames.Add("PhysicsCore");
        }
        else
        {
            PrivateDependencyModuleNames.Add("Chaos");
        }

        if (Target.bBuildEditor == true)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "UnrealEd",
                    "Slate",
                    "SlateCore",
                }
            );
        }

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "Private/PCH.h";
        CppStandard = CppStandardVersion.Cpp17;
        bEnableExceptions = true;
    }
}
