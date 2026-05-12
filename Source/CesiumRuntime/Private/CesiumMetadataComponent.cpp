// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumMetadataComponent.h"
#include "Cesium3DTileset.h"

#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>

void UCesiumMetadataComponent::SyncStatistics() {
  if (this->_syncInProgress)
    return;

  this->ClearStatistics();

  ACesium3DTileset* pActor = this->GetOwner<ACesium3DTileset>();
  Cesium3DTilesSelection::Tileset* pTileset =
      pActor ? pActor->GetTileset() : nullptr;

  if (pTileset && pTileset->getMetadata()) {
    this->OnFetchMetadata(pActor, pTileset->getMetadata());
  } else if (pTileset) {
    this->_syncInProgress = true;

    pTileset->loadMetadata()
        .thenInMainThread(
            [this,
             pActor](const Cesium3DTilesSelection::TilesetMetadata* pMetadata) {
              if (!IsValid(pActor) || !IsValid(this) ||
                  !this->_syncInProgress) {
                // _syncInProgress can be set to false if the sync is
                // intentionally interrupted.
                return;
              }
              this->OnFetchMetadata(pActor, pMetadata);
              this->_syncInProgress = false;
            })
        .catchInMainThread(
            [this](std::exception&& e) { this->_syncInProgress = false; });
  }
}

bool UCesiumMetadataComponent::IsSyncing() const {
  return this->_syncInProgress;
}

void UCesiumMetadataComponent::InterruptSync() {
  this->_syncInProgress = false;
  this->ClearStatistics();
}

void UCesiumMetadataComponent::OnRegister() {
  Super::OnRegister();
  this->SyncStatistics();
}

void UCesiumMetadataComponent::OnFetchMetadata(
    ACesium3DTileset* pActor,
    const Cesium3DTilesSelection::TilesetMetadata* pMetadata) {}

void UCesiumMetadataComponent::ClearStatistics() {}
