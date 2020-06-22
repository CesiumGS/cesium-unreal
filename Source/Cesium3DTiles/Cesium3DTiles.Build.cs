// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Cesium3DTiles : ModuleRules
{
    public Cesium3DTiles(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "../../ThirdParty/tinygltf"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/glm"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/GSL/include")
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
                "../ThirdParty/draco/src",
                "../ThirdParty/build/draco",
                "../ThirdParty/uriparser/include"
            }
            );

        PublicAdditionalLibraries.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "../../ThirdParty/build/draco/Release/dracodec.lib"),
                Path.Combine(ModuleDirectory, "../../ThirdParty/build/uriparser/Release/uriparser.lib")
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
				// ... add private dependencies that you statically link with here ...	
			}
            );

        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        PCHUsage = PCHUsageMode.NoSharedPCHs;
        PrivatePCHHeaderFile = "Private/PCH.h";
        CppStandard = CppStandardVersion.Cpp17;
    }
}
