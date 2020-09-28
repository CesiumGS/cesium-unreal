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
                "../extern/cesium-native/CesiumGeospatial/include",
                "../extern/cesium-native/CesiumGeometry/include",
                "../extern/cesium-native/CesiumUtility/include",
                "../extern/cesium-native/extern/tinygltf",
                "../extern/cesium-native/extern/glm",
                "../extern/cesium-native/extern/GSL/include",
            }
            );

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/build/Cesium3DTiles/Debug/Cesium3DTiles.lib"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/build/CesiumGeospatial/Debug/CesiumGeospatial.lib"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/build/CesiumGeometry/Debug/CesiumGeometry.lib"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/build/CesiumUtility/Debug/CesiumUtility.lib"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/build/uriparser/Release/uriparser.lib"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/build/draco/Release/draco.lib"),
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
                "HTTP"
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
}
