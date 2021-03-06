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

        string debugRelease = "";
        if (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            debugRelease = "-debug";
        }
        else
        {
            debugRelease = "-release";
        }

        string platform = Target.Platform == UnrealTargetPlatform.Win64
            ? "-Windows-64"
            : "-Unknown";

        string postfix = platform + debugRelease;

        string libPath = Path.Combine(ModuleDirectory, "../ThirdParty/lib");
        string searchSpec = "*" + postfix + ".lib";

        Console.WriteLine("Including " + searchSpec + " in " + libPath);

        string[] libs = Directory.GetFiles(libPath, searchSpec);
        PublicAdditionalLibraries.AddRange(libs);

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
