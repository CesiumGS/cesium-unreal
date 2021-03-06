// Copyright 2020-2021 CesiumGS, Inc. and Contributors

using UnrealBuildTool;
using System;
using System.IO;

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

        string platform = Target.Platform == UnrealTargetPlatform.Win64
            ? "Windows-x64"
            : "Unknown";

        string libPath = Path.Combine(ModuleDirectory, "../ThirdParty/lib/" + platform);

        string debugPostfix = (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame) ? "d" : "";

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(libPath, "Cesium3DTiles" + debugpostfix + ".lib"),
                Path.Combine(libPath, "CesiumAsync" + debugpostfix + ".lib"),
                Path.Combine(libPath, "CesiumGeospatial" + debugpostfix + ".lib"),
                Path.Combine(libPath, "CesiumGeometry" + debugpostfix + ".lib"),
                Path.Combine(libPath, "CesiumGltf" + debugpostfix + ".lib"),
                Path.Combine(libPath, "CesiumGltfReader" + debugpostfix + ".lib"),
                Path.Combine(libPath, "CesiumUtility" + debugpostfix + ".lib"),
                Path.Combine(libPath, "uriparser" + debugpostfix + ".lib"),
                Path.Combine(libPath, "draco" + debugpostfix + ".lib"),
                Path.Combine(libPath, "asyncplusplus" + debugpostfix + ".lib"),
                Path.Combine(libPath, "sqlite3" + debugpostfix + ".lib"),
                Path.Combine(libPath, "tinyxml2" + debugpostfix + ".lib"),
                Path.Combine(libPath, "spdlog" + debugpostfix + ".lib"),
                Path.Combine(libPath, "modp_b64" + debugpostfix + ".lib")
            }
        );

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
                // TODO: remove Slate dependency? Anything else?
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "MeshDescription",
                "StaticMeshDescription",
                "HTTP",
                "MikkTSpace",
                "Chaos",
                "LevelSequence"
                // ... add private dependencies that you statically link with here ...
            }
        );

        PublicDefinitions.AddRange(
            new string[]
            {
                "SPDLOG_COMPILED_LIB"
            }
        );

        if (Target.bCompilePhysX && !Target.bUseChaos)
        {
            PrivateDependencyModuleNames.Add("PhysXCooking");
            PrivateDependencyModuleNames.Add("PhysicsCore");
        }

        if (Target.bBuildEditor == true)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "UnrealEd",
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
    }
}
