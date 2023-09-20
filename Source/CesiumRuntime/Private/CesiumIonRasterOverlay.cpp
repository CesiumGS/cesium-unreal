// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTilesSelection/IonRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumActors.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

void UCesiumIonRasterOverlay::TroubleshootToken() {
  OnCesiumRasterOverlayIonTroubleshooting.Broadcast(this);
}

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumIonRasterOverlay::CreateOverlay(
    const Cesium3DTilesSelection::RasterOverlayOptions& options) {
  if (this->IonAssetID <= 0) {
    // Don't create an overlay for an invalid asset ID.
    return nullptr;
  }

  FString token =
      this->IonAccessToken.IsEmpty()
          ? GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken
          : this->IonAccessToken;
  if (!this->IonAssetEndpointUrl.IsEmpty()) {
    return std::make_unique<Cesium3DTilesSelection::IonRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        this->IonAssetID,
        TCHAR_TO_UTF8(*token),
        options,
        TCHAR_TO_UTF8(*this->IonAssetEndpointUrl));
  }
  return std::make_unique<Cesium3DTilesSelection::IonRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->IonAssetID,
      TCHAR_TO_UTF8(*token),
      options);
}

void UCesiumIonRasterOverlay::PostLoad() {
  Super::PostLoad();

#if WITH_EDITOR
  // Only fixup flags in the editor, when not in play mode
  // We want to exclude fixing up any objects that aren't in this level
  if (!GEditor->IsPlaySessionInProgress()) {
    CesiumActors::validateActorComponentFlags(this);
  }
#endif
}
