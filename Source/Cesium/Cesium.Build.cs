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
                "../ThirdParty/tinygltf",
                "../../../packages/draco.CPP.1.3.3.1/build/native/include"
            }
            );

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "../../../../packages/draco.CPP.1.3.3.1/build/native/lib/x64/Release/dracodec.lib")
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
                "StaticMeshDescription"
				// ... add private dependencies that you statically link with here ...	
			}
            );

        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

    }
}
