using UnrealBuildTool;
using System.Collections.Generic;

public class devEditorTarget : TargetRules
{
    public devEditorTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
#if UE_5_4_OR_LATER
        DefaultBuildSettings = BuildSettingsVersion.V4;
#else
        DefaultBuildSettings = BuildSettingsVersion.V2;
#endif
        ExtraModuleNames.AddRange( new string[] { "dev" } );
    }
}
