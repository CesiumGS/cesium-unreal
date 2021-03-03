// Copyright 2020-2021 CesiumGS, Inc. and Contributors

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
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/Cesium3DTiles/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/CesiumAsync/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/CesiumGeospatial/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/CesiumGeometry/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/CesiumGltf/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/CesiumGltfReader/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/CesiumUtility/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/glm"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/GSL/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/asyncplusplus/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/spdlog/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/rapidjson/include"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/stb"),
                Path.Combine(ModuleDirectory, "../../extern/cesium-native/extern/sqlite3"),
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
            }
            );

        string cesiumNativeConfiguration = "Debug";
        string tinyxml2Name = "tinyxml2d.lib";
        string spdlogLibName = "spdlogd.lib";
        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            cesiumNativeConfiguration = "RelWithDebInfo";
            tinyxml2Name = "tinyxml2.lib";
            spdlogLibName = "spdlog.lib";
        }

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/Cesium3DTiles/" + cesiumNativeConfiguration + "/Cesium3DTiles.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumAsync/" + cesiumNativeConfiguration + "/CesiumAsync.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumGeospatial/" + cesiumNativeConfiguration + "/CesiumGeospatial.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumGeometry/" + cesiumNativeConfiguration + "/CesiumGeometry.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumGltf/" + cesiumNativeConfiguration + "/CesiumGltf.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumGltfReader/" + cesiumNativeConfiguration + "/CesiumGltfReader.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumUtility/" + cesiumNativeConfiguration + "/CesiumUtility.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/uriparser/" + cesiumNativeConfiguration + "/uriparser.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/draco/" + cesiumNativeConfiguration + "/draco.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/asyncplusplus/" + cesiumNativeConfiguration + "/async++.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/sqlite3/" + cesiumNativeConfiguration + "/sqlite3.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/tinyxml2/" + cesiumNativeConfiguration + "/" + tinyxml2Name),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/spdlog/" + cesiumNativeConfiguration + "/" + spdlogLibName),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/modp_b64/" + cesiumNativeConfiguration + "/modp_b64.lib")
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
