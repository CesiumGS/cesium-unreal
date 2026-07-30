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

#include <atomic>
#include <vector>

namespace {
class BlueprintClassVectorStylingProvider
    : public CesiumVectorOverlays::VectorStylingProvider {
public:
  BlueprintClassVectorStylingProvider(UObject* pObject) {
    if (IsValid(pObject)) {
      this->_pInterface = pObject;
      if (this->IsInterfaceValid()) {
        this->_useWorkerThread =
            ICesiumVectorTilesStylingCallbacks::Execute_ShouldRunOnWorkerThread(
                this->_pInterface.GetObject());
      }
      this->_blueprintCompileDelegate = GEditor->OnBlueprintPreCompile().AddRaw(
          this,
          &BlueprintClassVectorStylingProvider::OnBlueprintPreCompile);
    }
  }

  virtual ~BlueprintClassVectorStylingProvider() {
    GEditor->OnBlueprintPreCompile().Remove(this->_blueprintCompileDelegate);
  }

  void OnBlueprintPreCompile(UBlueprint* pBlueprint) {
    if (this->_pInterface.GetObject() != nullptr &&
        pBlueprint->GeneratedClass ==
            this->_pInterface.GetObject()->GetClass()) {
      CesiumAsync::AsyncSystem& asyncSystem = getAsyncSystem();
      while (this->_pendingStylingJobs > 0) {
        asyncSystem.dispatchMainThreadTasks();
      }

      this->_pInterface = nullptr;
    }
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

    auto lambda = [this, Metadata, featureIds, points]() {
      std::vector<std::optional<CesiumVectorData::VectorStyle>> result;
      ++this->_pendingStylingJobs;
      if (!this->IsInterfaceValid()) {
        --this->_pendingStylingJobs;
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

      --this->_pendingStylingJobs;
      return result;
    };

    if (this->_useWorkerThread) {
      return asyncSystem.runInWorkerThread(lambda);
    } else {
      return asyncSystem.runInMainThread(lambda);
    }
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

    auto lambda = [this, Metadata, featureIds, polylines]() {
      std::vector<std::optional<CesiumVectorData::VectorStyle>> result;
      ++this->_pendingStylingJobs;
      if (!this->IsInterfaceValid()) {
        --this->_pendingStylingJobs;
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

      --this->_pendingStylingJobs;
      return result;
    };

    if (this->_useWorkerThread) {
      return asyncSystem.runInWorkerThread(lambda);
    } else {
      return asyncSystem.runInMainThread(lambda);
    }
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

    auto lambda = [this, Metadata, featureIds, polygons]() {
      std::vector<std::optional<CesiumVectorData::VectorStyle>> result;
      ++this->_pendingStylingJobs;
      if (!this->IsInterfaceValid()) {
        --this->_pendingStylingJobs;
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

      --this->_pendingStylingJobs;
      return result;
    };

    if (this->_useWorkerThread) {
      return asyncSystem.runInWorkerThread(lambda);
    } else {
      return asyncSystem.runInMainThread(lambda);
    }
  }

  inline bool IsInterfaceValid() {
    return this->_pInterface.GetObject() != nullptr &&
           !this->_pInterface.GetObject()->HasAnyFlags(
               EObjectFlags::RF_BeginDestroyed |
               EObjectFlags::RF_FinishDestroyed |
               EObjectFlags::RF_MirroredGarbage) &&
           IsValid(this->_pInterface.GetObject()) &&
           !this->_pInterface.GetObject()->GetClass()->HasAnyClassFlags(
               EClassFlags::CLASS_NewerVersionExists);
  }

private:
  bool _useWorkerThread = false;
  TScriptInterface<ICesiumVectorTilesStylingCallbacks> _pInterface = nullptr;
  FDelegateHandle _blueprintCompileDelegate;
  std::atomic<int> _pendingStylingJobs = 0;
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
  if (this->StylingProviderType == ECesiumVectorStylingProviderType::Blueprint && IsValid(this->BlueprintStylingProvider)) {
    if (!IsValid(this->_pStylingInterfaceObject)) {
      this->_pStylingInterfaceObject =
          NewObject<UObject>(this, this->BlueprintStylingProvider);
    }

    pStylingProvider = std::make_shared<BlueprintClassVectorStylingProvider>(
        this->_pStylingInterfaceObject);
  } else if (
      this->StylingProviderType == ECesiumVectorStylingProviderType::Lambda && this->LambdaStylingProvider.IsSet()) {
    pStylingProvider = this->LambdaStylingProvider.GetValue()();
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
