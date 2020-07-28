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
                "../ThirdParty/cesium-native/Cesium3DTiles/include",
                "../ThirdParty/cesium-native/CesiumGeospatial/include",
                "../ThirdParty/cesium-native/CesiumGeometry/include",
                "../ThirdParty/cesium-native/CesiumUtility/include",
                "../ThirdParty/cesium-native/extern/tinygltf",
                "../ThirdParty/cesium-native/extern/glm",
                "../ThirdParty/cesium-native/extern/GSL/include",
            }
            );

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "../../ThirdParty/cesium-native/build/Cesium3DTiles/Debug/Cesium3DTiles.lib"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/cesium-native/build/CesiumGeospatial/Debug/CesiumGeospatial.lib"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/cesium-native/build/CesiumGeometry/Debug/CesiumGeometry.lib"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/cesium-native/build/CesiumUtility/Debug/CesiumUtility.lib"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/cesium-native/extern/build/uriparser/Release/uriparser.lib"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/cesium-native/extern/build/draco/Release/draco.lib"),
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
                "UnrealEd", // TODO: only include this in editor builds?
                "PhysXCooking",
                "PhysicsCore"
				// ... add private dependencies that you statically link with here ...	
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
