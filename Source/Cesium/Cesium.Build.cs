// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Cesium : ModuleRules
{
    public Cesium(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
                "../extern/cesium-native/Cesium3DTiles/include",
                "../extern/cesium-native/CesiumAsync/include",
                "../extern/cesium-native/CesiumGeospatial/include",
                "../extern/cesium-native/CesiumGeometry/include",
                "../extern/cesium-native/CesiumUtility/include",
                "../extern/cesium-native/extern/tinygltf",
                "../extern/cesium-native/extern/glm",
                "../extern/cesium-native/extern/GSL/include",
                "../extern/cesium-native/extern/asyncplusplus/include"
            }
            );

        // string cesiumNativeConfiguration = "Debug";
        // string tinyxml2Name = "tinyxml2d.lib";
        // if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        // {
        //     cesiumNativeConfiguration = "RelWithDebInfo";
        //     tinyxml2Name = "tinyxml2.lib";
        // }

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/Cesium3DTiles/" + library("Cesium3DTiles", false)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumAsync/" + library("CesiumAsync", false)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumGeospatial/" + library("CesiumGeospatial", false)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumGeometry/" + library("CesiumGeometry", false)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumUtility/" + library("CesiumUtility", false)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/uriparser/" + library("uriparser", false)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/draco/" + library("draco", false)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/tinyxml2/" + library("tinyxml2", true)),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/asyncplusplus/" + library("async++", false))
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
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "MeshDescription",
                "StaticMeshDescription",
                "HTTP",
                "MikkTSpace",
                "Chaos"
				// ... add private dependencies that you statically link with here ...	
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

    private string library(string baseName, bool addDForDebug) {
        if (addDForDebug)
        {
            baseName += "d";
        }

        if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
        {
            if (Target.Configuration == UnrealTargetConfiguration.Shipping)
            {
                baseName = "RelWithDebInfo" + "/" + baseName;
            }
            else
            {
                baseName = "Debug" + "/" + baseName;
            }

            return baseName + ".lib";
        }
        else
        {
            return "lib" + baseName + ".a";
        }

    }
}
