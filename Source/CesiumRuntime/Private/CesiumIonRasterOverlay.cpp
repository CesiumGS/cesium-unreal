// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumActors.h"
#include "CesiumCustomVersion.h"
#include "CesiumIonServer.h"
#include "CesiumRasterOverlays/IonRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"

#if WITH_EDITOR
#include "FileHelpers.h"
#endif

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

  // Make sure we have a valid Cesium ion server.
  if (!IsValid(this->CesiumIonServer)) {
    this->CesiumIonServer = UCesiumIonServer::GetServerForNewObjects();
  }

  FString token = this->IonAccessToken.IsEmpty()
                      ? this->CesiumIonServer->DefaultIonAccessToken
                      : this->IonAccessToken;

#if WITH_EDITOR
  this->CesiumIonServer->ResolveApiUrl();
#endif

  // Make sure the URL ends with a slash
  std::string apiUrl = TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl);
  if (apiUrl.empty())
    return nullptr;

  if (*apiUrl.rbegin() != '/')
    apiUrl += '/';

  return std::make_unique<CesiumRasterOverlays::IonRasterOverlay>(
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
  if (CesiumVersion < FCesiumCustomVersion::CesiumIonServer) {
    this->CesiumIonServer = UCesiumIonServer::GetBackwardCompatibleServer(
        this->IonAssetEndpointUrl_DEPRECATED);
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
}
