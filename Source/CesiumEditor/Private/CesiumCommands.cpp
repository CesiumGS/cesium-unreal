// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCommands.h"
#include "CesiumEditor.h"

#define LOCTEXT_NAMESPACE "CesiumCommands"

FCesiumCommands::FCesiumCommands()
    : TCommands<FCesiumCommands>(
          "Cesium.Common",
          LOCTEXT("Common", "Common"),
          NAME_None,
          FCesiumEditorModule::GetStyleSetName()) {}

void FCesiumCommands::RegisterCommands() {
  UI_COMMAND(
      AddFromIon,
      "Add",
      "Add a tileset from Cesium ion to this level",
      EUserInterfaceActionType::Button,
      FInputChord());
  UI_COMMAND(
      UploadToIon,
      "Upload",
      "Upload a tileset to Cesium ion to process it for efficient streaming to Cesium for Unreal",
      EUserInterfaceActionType::Button,
      FInputChord());
  UI_COMMAND(
      SignOut,
      "Sign Out",
      "Sign out of Cesium ion",
      EUserInterfaceActionType::Button,
      FInputChord());
  UI_COMMAND(
      OpenDocumentation,
      "Learn",
      "Open Cesium for Unreal tutorials and learning resources",
      EUserInterfaceActionType::Button,
      FInputChord());
  UI_COMMAND(
      OpenSupport,
      "Help",
      "Search for existing questions or ask a new question on the Cesium Community Forum",
      EUserInterfaceActionType::Button,
      FInputChord());

  UI_COMMAND(
      OpenCesiumPanel,
      "Cesium",
      "Open the Cesium panel",
      EUserInterfaceActionType::Button,
      FInputChord());
}

#undef LOCTEXT_NAMESPACE
