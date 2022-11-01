// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumRasterOverlay.h"
#include "Async/Async.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTileset.h"
#include "CesiumAsync/IAssetResponse.h"

FCesiumRasterOverlayLoadFailure OnCesiumRasterOverlayLoadFailure{};

// Sets default values for this component's properties
UCesiumRasterOverlay::UCesiumRasterOverlay()
    : _pOverlay(nullptr), _overlaysBeingDestroyed(0) {
  this->bAutoActivate = true;

  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;

  // ...
}

#if WITH_EDITOR
// Called when properties are changed in the editor
void UCesiumRasterOverlay::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  this->Refresh();
}
#endif

void UCesiumRasterOverlay::AddToTileset() {
  if (this->_pOverlay) {
    return;
  }

  Cesium3DTilesSelection::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  Cesium3DTilesSelection::RasterOverlayOptions options{};
  options.maximumScreenSpaceError = this->MaximumScreenSpaceError;
  options.maximumSimultaneousTileLoads = this->MaximumSimultaneousTileLoads;
  options.maximumTextureSize = this->MaximumTextureSize;
  options.subTileCacheBytes = this->SubTileCacheBytes;
  options.showCreditsOnScreen = this->ShowCreditsOnScreen;
  options.rendererOptions = &this->rendererOptions;
  options.loadErrorCallback =
      [this](const Cesium3DTilesSelection::RasterOverlayLoadFailureDetails&
                 details) {
        static_assert(
            uint8_t(ECesiumRasterOverlayLoadType::CesiumIon) ==
            uint8_t(Cesium3DTilesSelection::RasterOverlayLoadType::CesiumIon));
        static_assert(
            uint8_t(ECesiumRasterOverlayLoadType::TileProvider) ==
            uint8_t(
                Cesium3DTilesSelection::RasterOverlayLoadType::TileProvider));
        static_assert(
            uint8_t(ECesiumRasterOverlayLoadType::Unknown) ==
            uint8_t(Cesium3DTilesSelection::RasterOverlayLoadType::Unknown));

        uint8_t typeValue = uint8_t(details.type);
        assert(
            uint8_t(details.type) <=
            uint8_t(
                Cesium3DTilesSelection::RasterOverlayLoadType::TilesetJson));
        assert(this->_pTileset == details.pTileset);

        FCesiumRasterOverlayLoadFailureDetails ueDetails{};
        ueDetails.Overlay = this;
        ueDetails.Type = ECesiumRasterOverlayLoadType(typeValue);
        ueDetails.HttpStatusCode =
            details.pRequest && details.pRequest->response()
                ? details.pRequest->response()->statusCode()
                : 0;
        ueDetails.Message = UTF8_TO_TCHAR(details.message.c_str());

        // Broadcast the event from the game thread.
        // Even if we're already in the game thread, let the stack unwind.
        // Otherwise actions that destroy the Tileset will cause a deadlock.
        AsyncTask(
            ENamedThreads::GameThread,
            [ueDetails = std::move(ueDetails)]() {
              OnCesiumRasterOverlayLoadFailure.Broadcast(ueDetails);
            });
      };

  std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> pOverlay =
      this->CreateOverlay(options);

  if (pOverlay) {
    this->_pOverlay = pOverlay.release();

    pTileset->getOverlays().add(this->_pOverlay);

    this->OnAdd(pTileset, this->_pOverlay);
  }
}

void UCesiumRasterOverlay::RemoveFromTileset() {
  if (!this->_pOverlay) {
    return;
  }

  Cesium3DTilesSelection::Tileset* pTileset = FindTileset();
  if (!pTileset) {
    return;
  }

  // Don't allow this RasterOverlay to be fully destroyed until
  // any cesium-native RasterOverlays it created have wrapped up any async
  // operations in progress and have been fully destroyed.
  // See IsReadyForFinishDestroy.
  ++this->_overlaysBeingDestroyed;
  this->_pOverlay->getAsyncDestructionCompleteEvent(getAsyncSystem())
      .thenInMainThread([this]() { --this->_overlaysBeingDestroyed; });

  this->OnRemove(pTileset, this->_pOverlay);
  pTileset->getOverlays().remove(this->_pOverlay);
  this->_pOverlay = nullptr;
}

void UCesiumRasterOverlay::Refresh() {
  this->RemoveFromTileset();
  this->AddToTileset();
}

double UCesiumRasterOverlay::GetMaximumScreenSpaceError() const {
  return this->MaximumScreenSpaceError;
}

void UCesiumRasterOverlay::SetMaximumScreenSpaceError(double Value) {
  this->MaximumScreenSpaceError = Value;
  this->Refresh();
}

int32 UCesiumRasterOverlay::GetMaximumTextureSize() const {
  return this->MaximumTextureSize;
}

void UCesiumRasterOverlay::SetMaximumTextureSize(int32 Value) {
  this->MaximumTextureSize = Value;
  this->Refresh();
}

int32 UCesiumRasterOverlay::GetMaximumSimultaneousTileLoads() const {
  return this->MaximumSimultaneousTileLoads;
}

void UCesiumRasterOverlay::SetMaximumSimultaneousTileLoads(int32 Value) {
  this->MaximumSimultaneousTileLoads = Value;

  if (this->_pOverlay) {
    this->_pOverlay->getOptions().maximumSimultaneousTileLoads = Value;
  }
}

int64 UCesiumRasterOverlay::GetSubTileCacheBytes() const {
  return this->SubTileCacheBytes;
}

void UCesiumRasterOverlay::SetSubTileCacheBytes(int64 Value) {
  this->SubTileCacheBytes = Value;

  if (this->_pOverlay) {
    this->_pOverlay->getOptions().subTileCacheBytes = Value;
  }
}

void UCesiumRasterOverlay::Activate(bool bReset) {
  Super::Activate(bReset);
  this->AddToTileset();
}

void UCesiumRasterOverlay::Deactivate() {
  Super::Deactivate();
  this->RemoveFromTileset();
}

void UCesiumRasterOverlay::OnComponentDestroyed(bool bDestroyingHierarchy) {
  this->RemoveFromTileset();
  Super::OnComponentDestroyed(bDestroyingHierarchy);
}

bool UCesiumRasterOverlay::IsReadyForFinishDestroy() {
  bool ready = Super::IsReadyForFinishDestroy();
  ready &= this->_overlaysBeingDestroyed == 0;

  if (!ready) {
    getAssetAccessor()->tick();
    getAsyncSystem().dispatchMainThreadTasks();
  }

  return ready;
}

Cesium3DTilesSelection::Tileset* UCesiumRasterOverlay::FindTileset() const {
  ACesium3DTileset* pActor = this->GetOwner<ACesium3DTileset>();
  if (!pActor) {
    return nullptr;
  }

  return pActor->GetTileset();
}
