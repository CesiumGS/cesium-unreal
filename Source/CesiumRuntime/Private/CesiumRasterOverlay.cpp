// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
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

  this->Refresh();
}
#endif

void UCesiumRasterOverlay::AddToTileset() {
  if (this->_pOverlay) {
    return;
  }

  Cesium3DTilesSelection::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  Cesium3DTilesSelection::RasterOverlayOptions options{};
  options.maximumScreenSpaceError = this->MaximumScreenSpaceError;
  options.maximumSimultaneousTileLoads = this->MaximumSimultaneousTileLoads;
  options.maximumTextureSize = this->MaximumTextureSize;
  options.subTileCacheBytes = this->SubTileCacheBytes;

  std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> pOverlay =
      this->CreateOverlay(options);
  this->_pOverlay = pOverlay.get();

  pTileset->getOverlays().add(std::move(pOverlay));

  this->OnAdd(pTileset, this->_pOverlay);
}

void UCesiumRasterOverlay::RemoveFromTileset() {
  if (!this->_pOverlay) {
    return;
  }

  Cesium3DTilesSelection::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  this->OnRemove(pTileset, this->_pOverlay);
  pTileset->getOverlays().remove(this->_pOverlay);
  this->_pOverlay = nullptr;
}

void UCesiumRasterOverlay::Refresh() {
  this->RemoveFromTileset();
  this->AddToTileset();
}

float UCesiumRasterOverlay::GetMaximumScreenSpaceError() const {
  return this->MaximumScreenSpaceError;
}

void UCesiumRasterOverlay::SetMaximumScreenSpaceError(float Value) {
  this->MaximumScreenSpaceError = Value;
  this->Refresh();
}

int32 UCesiumRasterOverlay::GetMaximumTextureSize() const {
  return this->MaximumTextureSize;
}

void UCesiumRasterOverlay::SetMaximumTextureSize(int32 Value) {
  this->MaximumTextureSize = Value;
  this->Refresh();
}

int32 UCesiumRasterOverlay::GetMaximumSimultaneousTileLoads() const {
  return this->MaximumSimultaneousTileLoads;
}

void UCesiumRasterOverlay::SetMaximumSimultaneousTileLoads(int32 Value) {
  this->MaximumSimultaneousTileLoads = Value;

  if (this->_pOverlay) {
    this->_pOverlay->getOptions().maximumSimultaneousTileLoads = Value;
  }
}

int64 UCesiumRasterOverlay::GetSubTileCacheBytes() const {
  return this->SubTileCacheBytes;
}

void UCesiumRasterOverlay::SetSubTileCacheBytes(int64 Value) {
  this->SubTileCacheBytes = Value;

  if (this->_pOverlay) {
    this->_pOverlay->getOptions().subTileCacheBytes = Value;
  }
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

Cesium3DTilesSelection::Tileset* UCesiumRasterOverlay::FindTileset() const {
  ACesium3DTileset* pActor = this->GetOwner<ACesium3DTileset>();
  if (!pActor) {
    return nullptr;
  }

  return pActor->GetTileset();
}
