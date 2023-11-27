// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTilesSelection/IonRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumActors.h"
#include "CesiumCustomVersion.h"
#include "CesiumIonServer.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"

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

  // Make sure we have a valid Cesium ion server.
  if (!IsValid(this->CesiumIonServer)) {
    this->CesiumIonServer = UCesiumIonServer::GetOrCreateDefault();
  }

  FString token = this->IonAccessToken.IsEmpty()
                      ? this->CesiumIonServer->DefaultIonAccessToken
                      : this->IonAccessToken;

  // Make sure the URL ends with a slash
  std::string apiUrl = TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl);
  if (!apiUrl.empty() && *apiUrl.rbegin() != '/')
    apiUrl += '/';

  return std::make_unique<Cesium3DTilesSelection::IonRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->IonAssetID,
      TCHAR_TO_UTF8(*token),
      options,
      apiUrl);
}

void UCesiumIonRasterOverlay::PostLoad() {
  Super::PostLoad();

  if (CesiumActors::shouldValidateFlags(this))
    CesiumActors::validateActorComponentFlags(this);

#if WITH_EDITOR
  const int32 CesiumVersion =
      this->GetLinkerCustomVersion(FCesiumCustomVersion::GUID);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  if (CesiumVersion < FCesiumCustomVersion::CesiumIonServer &&
      !this->IonAssetEndpointUrl_DEPRECATED.IsEmpty() &&
      this->IonAssetEndpointUrl_DEPRECATED !=
          TEXT("https://api.ion.cesium.com/")) {
    this->CesiumIonServer = UCesiumIonServer::GetOrCreateForApiUrl(
        this->IonAssetEndpointUrl_DEPRECATED);
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
}
