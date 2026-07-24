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
  BlueprintClassVectorStylingProvider(UObject* pInterface) {
    if (IsValid(pInterface)) {
      this->_pInterface = pInterface;
      RegisterGCObject();
    }
  }

  ~BlueprintClassVectorStylingProvider() {
    UnregisterGCObject();
    this->_pInterface = nullptr;
  }

  CesiumAsync::Future<std::vector<std::optional<CesiumVectorData::VectorStyle>>>
  onStylePoints(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<CesiumGeospatial::Cartographic>& points) override {
    const CesiumGltf::ExtensionModelExtStructuralMetadata* pStructuralMetadata =
        model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();

    FCesiumModelMetadata Metadata;
    if (pStructuralMetadata) {
      Metadata = FCesiumModelMetadata(model, *pStructuralMetadata);
    }

    return asyncSystem.createResolvedFuture<bool>(true).thenInMainThread(
        [this, Metadata, featureIds, points](bool /*result*/) {
          std::vector<std::optional<CesiumVectorData::VectorStyle>> result;
          if (!this->IsInterfaceValid()) {
            return result;
          }

          result.reserve(featureIds.size());

          ICesiumVectorTilesStylingCallbacks::Execute_OnStylingBegin(
              this->_pInterface.GetObject(),
              Metadata);

          FCesiumVectorStyle OutStyle;

          for (size_t i = 0; i < featureIds.size(); i++) {
            if (ICesiumVectorTilesStylingCallbacks::Execute_OnStylePoint(
                    this->_pInterface.GetObject(),
                    featureIds[i],
                    FVector(
                        points[i].longitude,
                        points[i].latitude,
                        points[i].height),
                    OutStyle)) {
              result.emplace_back(OutStyle.toNative());
            } else {
              result.emplace_back(std::nullopt);
            }
          }

          return result;
        });
  }

  CesiumAsync::Future<std::vector<std::optional<CesiumVectorData::VectorStyle>>>
  onStylePolylines(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polylines)
      override {
    const CesiumGltf::ExtensionModelExtStructuralMetadata* pStructuralMetadata =
        model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();

    FCesiumModelMetadata Metadata;
    if (pStructuralMetadata) {
      Metadata = FCesiumModelMetadata(model, *pStructuralMetadata);
    }

    return asyncSystem.createResolvedFuture<bool>(true).thenInMainThread(
        [this, Metadata, featureIds, polylines](bool /*result*/) {
          std::vector<std::optional<CesiumVectorData::VectorStyle>> result;
          if (!this->IsInterfaceValid()) {
            return result;
          }

          result.reserve(featureIds.size());

          ICesiumVectorTilesStylingCallbacks::Execute_OnStylingBegin(
              this->_pInterface.GetObject(),
              Metadata);

          FCesiumVectorStyle OutStyle;
          TArray<FVector> PolylineLlh;

          for (size_t i = 0; i < featureIds.size(); i++) {
            PolylineLlh.SetNum(polylines[i].size());
            for (size_t j = 0; j < polylines[i].size(); j++) {
              PolylineLlh[j] = FVector(
                  polylines[i][j].longitude,
                  polylines[i][j].latitude,
                  polylines[i][j].height);
            }

            if (ICesiumVectorTilesStylingCallbacks::Execute_OnStylePolyline(
                    this->_pInterface.GetObject(),
                    featureIds[i],
                    PolylineLlh,
                    OutStyle)) {
              result.emplace_back(OutStyle.toNative());
            } else {
              result.emplace_back(std::nullopt);
            }
          }

          return result;
        });
  }

  CesiumAsync::Future<std::vector<std::optional<CesiumVectorData::VectorStyle>>>
  onStylePolygons(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& model,
      const std::vector<int64_t>& featureIds,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polygons)
      override {
    const CesiumGltf::ExtensionModelExtStructuralMetadata* pStructuralMetadata =
        model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();

    FCesiumModelMetadata Metadata;
    if (pStructuralMetadata) {
      Metadata = FCesiumModelMetadata(model, *pStructuralMetadata);
    }

    return asyncSystem.createResolvedFuture<bool>(true).thenInMainThread(
        [this, Metadata, featureIds, polygons](bool /*result*/) {
          std::vector<std::optional<CesiumVectorData::VectorStyle>> result;
          if (!this->IsInterfaceValid()) {
            return result;
          }

          result.reserve(featureIds.size());

          ICesiumVectorTilesStylingCallbacks::Execute_OnStylingBegin(
              this->_pInterface.GetObject(),
              Metadata);

          FCesiumVectorStyle OutStyle;
          TArray<FVector> PolygonLlh;

          for (size_t i = 0; i < featureIds.size(); i++) {
            PolygonLlh.SetNum(polygons[i].size());
            for (size_t j = 0; j < polygons[i].size(); j++) {
              PolygonLlh[j] = FVector(
                  polygons[i][j].longitude,
                  polygons[i][j].latitude,
                  polygons[i][j].height);
            }

            if (ICesiumVectorTilesStylingCallbacks::Execute_OnStylePolygon(
                    this->_pInterface.GetObject(),
                    featureIds[i],
                    PolygonLlh,
                    OutStyle)) {
              result.emplace_back(OutStyle.toNative());
            } else {
              result.emplace_back(std::nullopt);
            }
          }

          return result;
        });
  }

  // Inherited via FGCObject
  void AddReferencedObjects(FReferenceCollector& Collector) override {
    if (this->IsInterfaceValid()) {
      Collector.AddReferencedObject(this->_pInterface.GetObjectRef());
    }
  }

  FString GetReferencerName() const override {
    return TEXT("BlueprintClassVectorStylingProvider");
  }

  inline bool IsInterfaceValid() {
    return !this->_pInterface.GetObject()->HasAnyFlags(
               EObjectFlags::RF_BeginDestroyed |
               EObjectFlags::RF_FinishDestroyed |
               EObjectFlags::RF_MirroredGarbage) &&
           IsValid(this->_pInterface.GetObject()) &&
           !this->_pInterface.GetObject()->GetClass()->HasAnyClassFlags(
               EClassFlags::CLASS_NewerVersionExists);
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

  std::shared_ptr<CesiumVectorOverlays::VectorStylingProvider>
      pStylingProvider = nullptr;
  if (IsValid(this->StylingProvider)) {
    pStylingProvider = std::make_shared<BlueprintClassVectorStylingProvider>(
        NewObject<UObject>(GetTransientPackage(), this->StylingProvider));
  }

  CesiumVectorOverlays::VectorTilesRasterOverlayOptions vectorOptions{
      this->DefaultStyle.toNative(),
      headers,
      pStylingProvider};

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
