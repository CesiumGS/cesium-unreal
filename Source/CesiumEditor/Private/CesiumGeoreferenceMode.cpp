// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceMode.h"

#include "Editor/UnrealEd/Public/Toolkits/ToolkitManager.h"
#include "ScopedTransaction.h"
#include "EditorViewportClient.h"
#include "UnrealClient.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"
#include <glm/mat3x3.hpp>
#include "InputCoreTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "CesiumEditor.h"
#include "CesiumGeoreferenceModeToolkit.h"
#include "CesiumGeoreference.h"

const FEditorModeID FCesiumGeoreferenceMode::EM_CesiumGeoreferenceMode(
    TEXT("EM_CesiumGeoreferenceMode"));

void FCesiumGeoreferenceMode::Enter() {
    FEdMode::Enter();

    if (!Toolkit.IsValid()) {
        Toolkit = MakeShareable(new FCesiumGeoreferenceModeToolkit);
        Toolkit->Init(Owner->GetToolkitHost());
    }

    if (!this->Georeference) {
      this->Georeference = ACesiumGeoreference::GetDefaultForLevel(
          this->GetWorld()->PersistentLevel);
    }
}

void FCesiumGeoreferenceMode::Exit() {
    FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
    Toolkit.Reset();
    
    FEdMode::Exit();
}

bool FCesiumGeoreferenceMode::InputDelta(
        FEditorViewportClient* InViewportClient,
        FViewport* InViewport,
        FVector& InDrag,
        FRotator& InRot,
        FVector& InScale) {
    return false;
}

bool FCesiumGeoreferenceMode::InputKey(
        FEditorViewportClient* InViewportClient,
        FViewport* InViewport,
        FKey Key,
        EInputEvent Event) {
  
  if (!this->Georeference) {
    return false;
  }
  
  FVector cameraLocation = InViewportClient->GetViewLocation();

  glm::dmat3 enuToUnreal = this->Georeference->ComputeEastNorthUpToUnreal(
      glm::dvec3(cameraLocation.X, cameraLocation.Y, cameraLocation.Z));

  double speed = 1000.0;
  glm::dvec3 delta(0.0, 0.0, 0.0);

  UE_LOG(LogTemp, Warning, TEXT("Pressed: %s"), *Key.ToString());

  if (Key == EKeys::W) { 
    delta = enuToUnreal * glm::dvec3(0.0, speed, 0.0);
  } else if (Key == EKeys::A) {
    delta = enuToUnreal * glm::dvec3(-speed, 0.0, 0.0);
  } else if (Key == EKeys::S) {
    delta = enuToUnreal * glm::dvec3(0.0, -speed, 0.0);
  } else if (Key == EKeys::D) {
    delta = enuToUnreal * glm::dvec3(speed, 0.0, 0.0);
  } else if (Key == EKeys::E) {
    delta = enuToUnreal * glm::dvec3(0.0, 0.0, speed);
  } else if (Key == EKeys::Q) {
    delta = enuToUnreal * glm::dvec3(0.0, 0.0, -speed);
  } else {
    //return false;
  }

  InViewportClient->SetViewLocation(
    cameraLocation + FVector(delta.x, delta.y, delta.z));

  return true;
}
