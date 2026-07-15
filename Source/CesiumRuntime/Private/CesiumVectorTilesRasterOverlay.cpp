// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumVectorTilesRasterOverlay.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumVectorData/VectorStyle.h>
#include <CesiumVectorOverlays/VectorStylingProvider.h>
#include <CesiumVectorOverlays/VectorTilesRasterOverlay.h>

#include "CesiumCustomVersion.h"
#include "CesiumRuntime.h"

#include <vector>

namespace {
class BlueprintClassVectorStylingProvider
    : public FGCObject,
      public CesiumVectorOverlays::VectorStylingProvider {
public:
  BlueprintClassVectorStylingProvider(UClass* pClass) {
    if (IsValid(pClass)) {
      this->_pInterface =
          NewObject<UObject>(GetTransientPackage(), pClass);
      RegisterGCObject();
    }
  }

  void onStylingBegin(const CesiumGltf::Model& model) override {
    if (!IsValid(this->_pInterface.GetObject())) {
      return;
    }

    const CesiumGltf::ExtensionModelExtStructuralMetadata* pStructuralMetadata =
        model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();

    if (!pStructuralMetadata) {
      return;
    }

    FCesiumModelMetadata Metadata(model, *pStructuralMetadata);
    ICesiumVectorTilesStylingCallbacks::Execute_OnStylingBegin(
        this->_pInterface.GetObject(),
        Metadata);
  }

  std::optional<CesiumVectorData::VectorStyle> onStylePoint(
      int64_t featureId,
      const CesiumGeospatial::Cartographic& point) override {
    if (!IsValid(this->_pInterface.GetObject())) {
      return std::nullopt;
    }

    FCesiumVectorStyle OutStyle;
    if (ICesiumVectorTilesStylingCallbacks::Execute_OnStylePoint(
            this->_pInterface.GetObject(),
            featureId,
            FVector(point.longitude, point.latitude, point.height),
            OutStyle)) {
      return OutStyle.toNative();
    }

    return std::nullopt;
  }

  std::optional<CesiumVectorData::VectorStyle> onStylePolyline(
      int64_t featureId,
      const std::vector<CesiumGeospatial::Cartographic>& polyline) override {
    if (!IsValid(this->_pInterface.GetObject())) {
      return std::nullopt;
    }

    TArray<FVector> PolylineLlh;
    PolylineLlh.SetNum(polyline.size());
    for (size_t i = 0; i < polyline.size(); i++) {
      PolylineLlh[i] = FVector(
          polyline[i].longitude,
          polyline[i].latitude,
          polyline[i].height);
    }

    FCesiumVectorStyle OutStyle;
    if (ICesiumVectorTilesStylingCallbacks::Execute_OnStylePolyline(
            this->_pInterface.GetObject(),
            featureId,
            PolylineLlh,
            OutStyle)) {
      return OutStyle.toNative();
    }

    return std::nullopt;
  }

  std::optional<CesiumVectorData::VectorStyle> onStylePolygon(
      int64_t featureId,
      const std::vector<CesiumGeospatial::Cartographic>& polygon) override {
    if (!IsValid(this->_pInterface.GetObject())) {
      return std::nullopt;
    }

    TArray<FVector> PolygonLlh;
    PolygonLlh.SetNum(polygon.size());
    for (size_t i = 0; i < polygon.size(); i++) {
      PolygonLlh[i] =
          FVector(polygon[i].longitude, polygon[i].latitude, polygon[i].height);
    }

    FCesiumVectorStyle OutStyle;
    if (ICesiumVectorTilesStylingCallbacks::Execute_OnStylePolygon(
            this->_pInterface.GetObject(),
            featureId,
            PolygonLlh,
            OutStyle)) {
      return OutStyle.toNative();
    }

    return std::nullopt;
  }

  // Inherited via FGCObject
  void AddReferencedObjects(FReferenceCollector& Collector) override {
    if (IsValid(this->_pInterface.GetObject())) {
      Collector.AddReferencedObject(this->_pInterface.GetObjectRef());
    }
  }

  FString GetReferencerName() const override {
    return TEXT("BlueprintClassVectorStylingProvider");
  }

private:
  TScriptInterface<ICesiumVectorTilesStylingCallbacks> _pInterface = nullptr;
};
} // namespace

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
      headers,
      [pProviderClass = this->StylingProvider]() {
        return std::make_shared<BlueprintClassVectorStylingProvider>(
            pProviderClass);
      }};

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
