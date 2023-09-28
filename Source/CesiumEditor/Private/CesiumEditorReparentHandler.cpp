// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumEditorReparentHandler.h"
#include "Cesium3DTileset.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumSubLevelComponent.h"
#include "Engine/Engine.h"

CesiumEditorReparentHandler::CesiumEditorReparentHandler() {
  if (GEngine) {
    this->_subscription = GEngine->OnLevelActorAttached().AddRaw(
        this,
        &CesiumEditorReparentHandler::OnLevelActorAttached);
  }
}

CesiumEditorReparentHandler::~CesiumEditorReparentHandler() {
  if (GEngine) {
    GEngine->OnLevelActorAttached().Remove(this->_subscription);
    this->_subscription.Reset();
  }
}

void CesiumEditorReparentHandler::OnLevelActorAttached(
    AActor* Actor,
    const AActor* Parent) {
  ACesium3DTileset* Tileset = Cast<ACesium3DTileset>(Actor);
  if (IsValid(Tileset)) {
    Tileset->InvalidateResolvedGeoreference();
  }

  UCesiumGlobeAnchorComponent* GlobeAnchor =
      Actor->FindComponentByClass<UCesiumGlobeAnchorComponent>();
  if (IsValid(GlobeAnchor)) {
    GlobeAnchor->ResolveGeoreference(true);
  }

  UCesiumSubLevelComponent* SubLevel =
      Actor->FindComponentByClass<UCesiumSubLevelComponent>();
  if (IsValid(SubLevel)) {
    SubLevel->ResolveGeoreference(true);
  }
}
