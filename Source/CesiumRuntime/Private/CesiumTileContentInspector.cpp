#include "CesiumTileContentInspector.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include <cassert>
#include <spdlog/spdlog.h>

static std::shared_ptr<CesiumAsync::ITaskProcessor> pTaskProcessor =
    std::make_shared<UnrealTaskProcessor>();
static std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
    std::make_shared<UnrealAssetAccessor>();
static CesiumAsync::AsyncSystem inspectorAsyncSystem(pTaskProcessor);

ACesiumTileContentInspectorActor::ACesiumTileContentInspectorActor() 
: _pComponent{nullptr}, _isInit{false} {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = true;
  PrimaryActorTick.TickGroup = ETickingGroup::TG_PostUpdateWork;
}

const FString& ACesiumTileContentInspectorActor::GetUrl() const { return Url; }

void ACesiumTileContentInspectorActor::SetUrl(const FString &InUrl) { 
  if (Url != InUrl) {
    Url = InUrl;
  }
}

bool ACesiumTileContentInspectorActor::ShouldTickIfViewportsOnly() const {
  return true;
}

void ACesiumTileContentInspectorActor::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  if (!_isInit) {
    LoadContent();
    _isInit = true;
  }
  inspectorAsyncSystem.dispatchMainThreadTasks();
}

void ACesiumTileContentInspectorActor::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  FName PropName = NAME_None;
  if (PropertyChangedEvent.Property) {
    PropName = PropertyChangedEvent.Property->GetFName();
  }

  if (PropName ==
      GET_MEMBER_NAME_CHECKED(ACesiumTileContentInspectorActor, Url)) {
    LoadContent(); 
  }
}

void ACesiumTileContentInspectorActor::LoadContent() {
  pAssetAccessor->requestAsset(inspectorAsyncSystem, std::string(TCHAR_TO_UTF8(*Url)))
      .thenInWorkerThread(
          [](std::shared_ptr<CesiumAsync::IAssetRequest> request) -> std::unique_ptr<UCesiumGltfComponent::HalfConstructed> {
            if (!request->response()) {
              return nullptr;
            }

            const CesiumAsync::IAssetResponse*pResponse = request->response(); 
            if (pResponse->statusCode() != 0 &&
                (pResponse->statusCode() < 200 ||
                 pResponse->statusCode() >= 300)) {
              return nullptr;
            }

            Cesium3DTiles::BoundingVolume tileBoundingVolume =
                CesiumGeometry::BoundingSphere(glm::dvec3(0.0), 1.0);
            Cesium3DTiles::TilesetContentOptions options;
            gsl::span<const std::byte> data = request->response()->data();
            Cesium3DTiles::TileContentLoadInput loadInput(
                spdlog::default_logger(),
                data,
                "",
                "",
                "",
                tileBoundingVolume,
                std::nullopt,
                Cesium3DTiles::TileRefine::Replace,
                0.0,
                glm::dmat4(1.0),
                options);

            auto pResult = Cesium3DTiles::TileContentFactory::createContent(loadInput);
            if (!pResult || !pResult->model) {
              return nullptr;
            }

            return UCesiumGltfComponent::CreateOffGameThread(
                *pResult->model,
                glm::dmat4(1.0),
                nullptr);
          })
      .thenInMainThread(
          [this](std::unique_ptr<UCesiumGltfComponent::HalfConstructed> &&pResult) {
            if (!pResult) {
              return;
            }

            this->_pComponent = UCesiumGltfComponent::CreateOnGameThread(
                this,
                std::move(pResult),
                glm::dmat4(1.0),
                nullptr,
                nullptr,
                nullptr);
            this->_pComponent->SetVisibility(true, true);
        });
}

