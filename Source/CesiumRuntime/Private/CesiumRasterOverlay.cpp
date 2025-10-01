// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumRasterOverlay.h"
#include "Async/Async.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTileset.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h"
#include "CesiumRuntime.h"
#include "UnrealAssetAccessor.h"

FCesiumRasterOverlayLoadFailure OnCesiumRasterOverlayLoadFailure{};

// Sets default values for this component's properties
UCesiumRasterOverlay::UCesiumRasterOverlay()
    : _pOverlay(nullptr), _overlaysBeingDestroyed(0) {
  this->bAutoActivate = true;

  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;

  // Allow DestroyComponent to be called from Blueprints by anyone. Without
  // this, only the Actor (Cesium3DTileset) itself can destroy raster overlays.
  // That's really annoying because it's fairly common to dynamically add/remove
  // overlays at runtime.
  bAllowAnyoneToDestroyMe = true;
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

  CesiumRasterOverlays::RasterOverlayOptions options{};
  options.ellipsoid = pTileset->getOptions().ellipsoid;
  options.maximumScreenSpaceError = this->MaximumScreenSpaceError;
  options.maximumSimultaneousTileLoads = this->MaximumSimultaneousTileLoads;
  options.maximumTextureSize = this->MaximumTextureSize;
  options.subTileCacheBytes = this->SubTileCacheBytes;
  options.showCreditsOnScreen = this->ShowCreditsOnScreen;
  options.rendererOptions = &this->rendererOptions;
  options.loadErrorCallback =
      [this](
          const CesiumRasterOverlays::RasterOverlayLoadFailureDetails&
              details) {
        static_assert(
            uint8_t(ECesiumRasterOverlayLoadType::CesiumIon) ==
            uint8_t(CesiumRasterOverlays::RasterOverlayLoadType::CesiumIon));
        static_assert(
            uint8_t(ECesiumRasterOverlayLoadType::TileProvider) ==
            uint8_t(CesiumRasterOverlays::RasterOverlayLoadType::TileProvider));
        static_assert(
            uint8_t(ECesiumRasterOverlayLoadType::Unknown) ==
            uint8_t(CesiumRasterOverlays::RasterOverlayLoadType::Unknown));

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

  std::unique_ptr<CesiumRasterOverlays::RasterOverlay> pOverlay =
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
  this->_pOverlay.reset();
}

void UCesiumRasterOverlay::Refresh() {
  this->RemoveFromTileset();
  if (this->IsActive()) {
    this->AddToTileset();
  }
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

  // If the request has not ended yet, actively cancel the request
  {
    FScopeLock lock(&UnrealAssetAccessor::_pendingRequestsLock);
    const FString originBaseUrl = ExtractCleanBaseUrl(_url);
    for (const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& pRequest :
         UnrealAssetAccessor::_pendingRequests) {
      EHttpRequestStatus::Type status = pRequest->GetStatus();
      if (status == EHttpRequestStatus::NotStarted ||
          status == EHttpRequestStatus::Processing) {
        const FString& requestUrl = pRequest->GetURL();
        const FString requestBaseUrl = ExtractCleanBaseUrl(requestUrl);

        UE_LOG(
            LogTemp,
            Log,
            TEXT("WMTS URL = %s, Request URL = %s"),
            *_url,
            *requestUrl);
        if (_url == TEXT("https://dev.virtualearth.net") ||
            _url == TEXT("https://api.cesium.com")) {
          // bing map
          if (requestUrl.Contains(TEXT("tiles.virtualearth.net/tiles"))) {
            pRequest->CancelRequest();
          }
        } else if (originBaseUrl == requestBaseUrl) {
          // other map(china Tianditu map\mapbox sate1lite map\arcgis online
          // world map\and other map(with kvp or restful))
          pRequest->CancelRequest();
        }
      }
    }
  }

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
