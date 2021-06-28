using UnrealBuildTool;
using System.Collections.Generic;

public class devEditorTarget : TargetRules
{
    public devEditorTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange( new string[] { "dev" } );
    }
}