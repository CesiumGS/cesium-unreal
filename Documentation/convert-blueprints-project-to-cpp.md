Unreal Engine does not allow a Blueprints-only project to have an embedded C++ plugin like Cesium for Unreal. Fortunately, it's easy to convert a Blueprints project to a C++ project just by adding a few files.

* Create the `C:\Dev\cesium-unreal-samples\Source` directory and add a new file called `dev.Target.cs` to it with the following content:

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

* Create another new file called `devEditor.Target.cs` and add the following content to it:

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

* Create a subdirectory of that `Source` directory called `dev`, and create a new file called `dev.Build.cs` in it wtih the following content:

      using UnrealBuildTool;

      public class dev : ModuleRules
      {
            public dev(ReadOnlyTargetRules Target) : base(Target)
            {
                  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
                  PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
            }
      }

* Finally, create a new file called `dev.cpp` in the same `dev` subdirectory, with this content:

      #include "Modules/ModuleManager.h"

      IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, dev, "dev" );

Your project should now work as a C++ project. However, you probably do not want to commit this change to your project's source code repository. A project that includes C++ code like this will require everyone opening the project to have an installed and working C++ compiler, including e.g. artists that do not typically have such an environment.
