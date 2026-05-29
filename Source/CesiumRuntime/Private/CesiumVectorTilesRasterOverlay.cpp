// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVectorTilesRasterOverlay.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumVectorData/VectorStyle.h>
#include <CesiumVectorOverlays/VectorTilesRasterOverlay.h>

#include "CesiumCustomVersion.h"
#include "CesiumRuntime.h"

#include <vector>

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumVectorTilesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  std::vector<CesiumAsync::IAssetAccessor::THeader> headers;
  headers.reserve(this->RequestHeaders.Num());

  for (auto& [k, v] : this->RequestHeaders) {
    headers.push_back({TCHAR_TO_UTF8(*k), TCHAR_TO_UTF8(*v)});
  }

  CesiumVectorOverlays::VectorTilesRasterOverlayOptions vectorOptions{
      this->DefaultStyle.toNative(),
      headers};

  if (this->Source == ECesiumVectorTilesRasterOverlaySource::FromCesiumIon) {
    if (this->IonAssetID <= 0) {
      return nullptr;
    }

    if (!IsValid(this->CesiumIonServer)) {
      this->CesiumIonServer = UCesiumIonServer::GetServerForNewObjects();
    }

    FString token = this->IonAccessToken.IsEmpty()
                        ? this->CesiumIonServer->DefaultIonAccessToken
                        : this->IonAccessToken;

    return std::make_unique<CesiumVectorOverlays::VectorTilesRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        this->IonAssetID,
        TCHAR_TO_UTF8(*token),
        std::string(TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl)) + "/",
        vectorOptions,
        options);
  }

  if (this->Url.IsEmpty()) {
    return nullptr;
  }

  return std::make_unique<CesiumVectorOverlays::VectorTilesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Url),
      vectorOptions,
      options);
}
