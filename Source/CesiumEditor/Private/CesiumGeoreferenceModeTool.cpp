// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceModeTool.h"

#include "CesiumEditor.h"
#include "CesiumGeoreferenceMode.h"
#include "Misc/Paths.h"

//#define IMAGE_BRUSH(RelativePath, ...)
// FSlateImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")),
//__VA_ARGS__)

TSharedPtr<FSlateStyleSet> CesiumGeoreferenceModeTool::StyleSet = nullptr;

void CesiumGeoreferenceModeTool::OnStartupModule() {
  RegisterStyleSet();
  RegisterEditorMode();
}

void CesiumGeoreferenceModeTool::OnShutdownModule() {
  UnregisterStyleSet();
  UnregisterEditorMode();
}

void CesiumGeoreferenceModeTool::RegisterStyleSet() {
  // Const icon sizes
  const FVector2D Icon20x20(20.0f, 20.0f);
  const FVector2D Icon40x40(40.0f, 40.0f);

  // Only register once
  if (StyleSet.IsValid()) {
    return;
  }

  StyleSet =
      MakeShareable(new FSlateStyleSet("CesiumGeoreferenceModeToolStyle"));
  StyleSet->SetContentRoot(
      FPaths::ProjectDir() / TEXT("Content/EditorResources"));
  StyleSet->SetCoreContentRoot(
      FPaths::ProjectDir() / TEXT("Content/EditorResources"));

  // Spline editor
  {
    StyleSet->Set(
        "ExampleEdMode",
        new IMAGE_BRUSH(TEXT("IconExampleEditorMode"), Icon40x40));
    StyleSet->Set(
        "ExampleEdMode.Small",
        new IMAGE_BRUSH(TEXT("IconExampleEditorMode"), Icon20x20));
  }

  FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void CesiumGeoreferenceModeTool::UnregisterStyleSet() {
  if (StyleSet.IsValid()) {
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
    ensure(StyleSet.IsUnique());
    StyleSet.Reset();
  }
}

void CesiumGeoreferenceModeTool::RegisterEditorMode() {
  FEditorModeRegistry::Get().RegisterMode<FCesiumGeoreferenceMode>(
      FCesiumGeoreferenceMode::EM_CesiumGeoreferenceMode,
      FText::FromString("Cesium Georeference Mode"),
      FSlateIcon(
          StyleSet->GetStyleSetName(),
          "ExampleEdMode",
          "ExampleEdMode.Small"),
      true,
      500);
}

void CesiumGeoreferenceModeTool::UnregisterEditorMode() {
  FEditorModeRegistry::Get().UnregisterMode(
      FCesiumGeoreferenceMode::EM_CesiumGeoreferenceMode);
}

//#undef IMAGE_BRUSH