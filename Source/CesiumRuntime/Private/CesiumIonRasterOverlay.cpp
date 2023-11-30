// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumActors.h"
#include "CesiumRasterOverlays/IonRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"

void UCesiumIonRasterOverlay::TroubleshootToken() {
  OnCesiumRasterOverlayIonTroubleshooting.Broadcast(this);
}

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumIonRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->IonAssetID <= 0) {
    // Don't create an overlay for an invalid asset ID.
    return nullptr;
  }

  FString token =
      this->IonAccessToken.IsEmpty()
          ? GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken
          : this->IonAccessToken;
  if (!this->IonAssetEndpointUrl.IsEmpty()) {
    return std::make_unique<CesiumRasterOverlays::IonRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        this->IonAssetID,
        TCHAR_TO_UTF8(*token),
        options,
        TCHAR_TO_UTF8(*this->IonAssetEndpointUrl));
  }
  return std::make_unique<CesiumRasterOverlays::IonRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->IonAssetID,
      TCHAR_TO_UTF8(*token),
      options);
}

void UCesiumIonRasterOverlay::PostLoad() {
  Super::PostLoad();

  if (CesiumActors::shouldValidateFlags(this))
    CesiumActors::validateActorComponentFlags(this);
}
