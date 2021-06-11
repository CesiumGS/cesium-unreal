using UnrealBuildTool;
using System.Collections.Generic;

public class devTarget : TargetRules
{
    public devTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange( new string[] { "dev" } );
    }
}