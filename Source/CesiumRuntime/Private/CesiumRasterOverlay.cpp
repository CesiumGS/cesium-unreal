// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRasterOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTileset.h"

// Sets default values for this component's properties
UCesiumRasterOverlay::UCesiumRasterOverlay() {
  this->bAutoActivate = true;

  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;

  // ...
}

#if WITH_EDITOR
// Called when properties are changed in the editor
void UCesiumRasterOverlay::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  this->RemoveFromTileset();
  this->AddToTileset();
}
#endif

void UCesiumRasterOverlay::AddToTileset() {
  if (this->_pOverlay) {
    return;
  }

  Cesium3DTiles::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  std::unique_ptr<Cesium3DTiles::RasterOverlay> pOverlay =
      this->CreateOverlay();
  this->_pOverlay = pOverlay.get();

  pTileset->getOverlays().add(std::move(pOverlay));
}

void UCesiumRasterOverlay::RemoveFromTileset() {
  if (!this->_pOverlay) {
    return;
  }

  Cesium3DTiles::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  pTileset->getOverlays().remove(this->_pOverlay);
  this->_pOverlay = nullptr;
}

void UCesiumRasterOverlay::Activate(bool bReset) {
  Super::Activate(bReset);
  this->AddToTileset();
}

void UCesiumRasterOverlay::Deactivate() {
  Super::Deactivate();
  this->RemoveFromTileset();
}

void UCesiumRasterOverlay::OnComponentDestroyed(bool bDestroyingHierarchy) {
  this->RemoveFromTileset();
  Super::OnComponentDestroyed(bDestroyingHierarchy);
}

Cesium3DTiles::Tileset* UCesiumRasterOverlay::FindTileset() const {
  ACesium3DTileset* pActor = this->GetOwner<ACesium3DTileset>();
  if (!pActor) {
    return nullptr;
  }

  return pActor->GetTileset();
}
