// Copyright 2020-2021 CesiumGS, Inc. and Contributors

using UnrealBuildTool;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

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

        string libPrefix;
        string libPostfix;
        string platform;
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            platform = "Windows-x64";
            libPostfix = ".lib";
            libPrefix = "";
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            platform = "Darwin-x64";
            libPostfix = ".a";
            libPrefix = "lib";
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            platform = "Android-xaarch64";
            libPostfix = ".a";
            libPrefix = "lib";
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            platform = "Linux-x64";
            libPostfix = ".a";
            libPrefix = "lib";
        }
        else if(Target.Platform == UnrealTargetPlatform.IOS)
        {
            platform = "iOS-xarm64";
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
            "async++",
            "Cesium3DTilesSelection",
            "CesiumAsync",
            "CesiumGeometry",
            "CesiumGeospatial",
            "CesiumGltfReader",
            "CesiumGltf",
            "CesiumJsonReader",
            "CesiumUtility",
            "draco",
            "ktx_read",
            //"MikkTSpace",
            "modp_b64",
            "s2geometry",
            "spdlog",
            "sqlite3",
            "tinyxml2",
            "uriparser",
            "webpdecoder",
            "ktx_read",
        };

        // Use our own copy of MikkTSpace on Android.
        if (Target.Platform == UnrealTargetPlatform.Android || Target.Platform == UnrealTargetPlatform.IOS)
        {
            libs = libs.Concat(new string[] { "MikkTSpace" }).ToArray();
            PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/include/mikktspace"));
        }

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            libs = libs.Concat(new string[] { "tidy_static" }).ToArray();
        }
        else
        {
            libs = libs.Concat(new string[] { "tidy" }).ToArray();
        }

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
                "Projects",
                "RenderCore",
                "SunPosition",
                "DeveloperSettings",
                "UMG",
                "Renderer"
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
                "SPDLOG_COMPILED_LIB",
                "LIBASYNC_STATIC",
                "GLM_FORCE_XYZW_ONLY",
                "GLM_FORCE_EXPLICIT_CTOR",
                "GLM_FORCE_SIZE_T_LENGTH",
                "TIDY_STATIC"
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
                    "WorldBrowser",
                    "ContentBrowser",
                    "MaterialEditor"
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
