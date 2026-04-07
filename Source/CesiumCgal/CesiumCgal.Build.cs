using UnrealBuildTool;
using System.IO;

public class CesiumCgal : ModuleRules
{
    public CesiumCgal(ReadOnlyTargetRules Target) : base(Target)
    {
        // No PCH and no unity build — each .cpp compiles in its own TU,
        // preventing UE's `check` macro (from ModuleManager.h) from
        // leaking into the CGAL/Boost translation unit.
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;

        // CGAL requires RTTI (typeid, dynamic_cast).
        bUseRTTI = true;
        bEnableExceptions = true;
        CppStandard = CppStandardVersion.Cpp20;

        // CGAL/Boost headers use `#if MACRO` on potentially-undefined macros.
        UnsafeTypeCastWarningLevel = WarningLevel.Off;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string HomebrewPrefix = "/opt/homebrew/opt";
            PublicIncludePaths.Add(Path.Combine(HomebrewPrefix, "cgal", "include"));
            PublicIncludePaths.Add(Path.Combine(HomebrewPrefix, "boost", "include"));
            PublicIncludePaths.Add(Path.Combine(HomebrewPrefix, "gmp", "include"));
            PublicIncludePaths.Add(Path.Combine(HomebrewPrefix, "mpfr", "include"));
            PublicIncludePaths.Add(Path.Combine(HomebrewPrefix, "eigen", "include", "eigen3"));
            PublicAdditionalLibraries.Add(Path.Combine(HomebrewPrefix, "gmp", "lib", "libgmp.a"));
            PublicAdditionalLibraries.Add(Path.Combine(HomebrewPrefix, "mpfr", "lib", "libmpfr.a"));
        }
    }
}
