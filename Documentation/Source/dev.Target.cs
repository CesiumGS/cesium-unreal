using UnrealBuildTool;
using System.Collections.Generic;

public class devTarget : TargetRules
{
    public devTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
#if UE_5_4_OR_LATER
        DefaultBuildSettings = BuildSettingsVersion.V4;
#else
        DefaultBuildSettings = BuildSettingsVersion.V2;
#endif
        ExtraModuleNames.AddRange( new string[] { "dev" } );
    }
}
