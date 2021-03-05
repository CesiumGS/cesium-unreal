// Copyright 2020-2021 CesiumGS, Inc. and Contributors

using UnrealBuildTool;
using System.IO;

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
                "../extern/cesium-native/CesiumIonClient/include",
            }
            );

        string cesiumNativeConfiguration = "Debug";
        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            cesiumNativeConfiguration = "RelWithDebInfo";
        }

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/CesiumIonClient/" + cesiumNativeConfiguration + "/CesiumIonClient.lib"),
                Path.Combine(ModuleDirectory, "../../extern/build/cesium-native/extern/" + cesiumNativeConfiguration + "/csprng.lib")
            }
            );

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
                "Slate",
                "SlateCore",
                "MeshDescription",
                "StaticMeshDescription",
                "HTTP",
                "MikkTSpace",
                "Chaos",
                "Projects",
                "InputCore",
                "PropertyEditor"
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
        PrivatePCHHeaderFile = "PCH.h";
        CppStandard = CppStandardVersion.Cpp17;
    }
}
