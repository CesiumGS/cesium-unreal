
#include "CesiumViewExtension.h"

#include "Cesium3DTileset.h"

CesiumViewExtension::CesiumViewExtension(const FAutoRegister& autoRegister)
    : FSceneViewExtensionBase(autoRegister) {}

CesiumViewExtension::~CesiumViewExtension() {}

void CesiumViewExtension::RegisterTileset(ACesium3DTileset* pTileset) {
  // TODO: add new tilesets to a temp list and add them the next time
  // the fence is complete in BeginRenderViewFamily
  // The current approach may cause a hitch when adding new tilesets.
  if (_aggregatingViewFamilies) {
    _finishAggregation();
  }
  
  _aggregationFence.Wait();
  this->_registeredTilesets.insert(pTileset);
}

// TODO: find a way not to block game thread on UnregisterTileset
void CesiumViewExtension::UnregisterTileset(ACesium3DTileset* pTileset) {
  if (_aggregatingViewFamilies) {
    _finishAggregation();
  }

  _aggregationFence.Wait();
  this->_registeredTilesets.erase(pTileset);
}

void CesiumViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily) {}

void CesiumViewExtension::SetupView(
    FSceneViewFamily& InViewFamily,
    FSceneView& InView) {}

void CesiumViewExtension::BeginRenderViewFamily(
    FSceneViewFamily& InViewFamily) {
  if (_frameNumber != (uint64_t)GFrameNumber) {
    // This is the first BeginRenderViewFamily call this frame. Effectively we 
    // use this as a "Tick".
    if (_aggregatingViewFamilies) {
      // Completed aggregating views from last frame, kick off a render fence
      // to wait for the render thread aggregation to finish 
      _finishAggregation();
    } else if (_aggregationFence.IsFenceComplete()) {
      if (_frameNumber > -1) {
        for (ACesium3DTileset* pTileset : this->_registeredTilesets) {
          pTileset->CompleteViewsAggregation();
        }
      }

      _startAggregation();      
    }
    
    _frameNumber = GFrameNumber;
  }
}

void CesiumViewExtension::PreRenderViewFamily_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FSceneViewFamily& InViewFamily) {}

void CesiumViewExtension::PreRenderView_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FSceneView& InView) {}

void CesiumViewExtension::PostRenderViewFamily_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FSceneViewFamily& InViewFamily) {
  if (_aggregatingViewFamilies_RenderThread) {
    for (ACesium3DTileset* pTileset : this->_registeredTilesets) {
      pTileset->AggregateViews_RenderThread(InViewFamily);
    }
  }
}

void CesiumViewExtension::_startAggregation() {
  _aggregatingViewFamilies = true;
  ENQUEUE_RENDER_COMMAND(CesiumViewExtension_StartAggregation)(
  [this](FRHICommandListImmediate& RHICmdList) {
    this->_aggregatingViewFamilies_RenderThread = true;
  });
}

void CesiumViewExtension::_finishAggregation() {
  _aggregatingViewFamilies = false;
  ENQUEUE_RENDER_COMMAND(CesiumViewExtension_FinishAggregation)
  ([this](FRHICommandListImmediate& RHICmdList) {
    this->_aggregatingViewFamilies_RenderThread = false;
  });
  _aggregationFence.BeginFence();
}
