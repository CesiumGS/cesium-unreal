
#include "CesiumViewExtension.h"

#include "Cesium3DTileset.h"

using namespace Cesium3DTilesSelection;

CesiumViewExtension::CesiumViewExtension(const FAutoRegister& autoRegister)
    : FSceneViewExtensionBase(autoRegister) {}

CesiumViewExtension::~CesiumViewExtension() {}

TileOcclusionState CesiumViewExtension::getPrimitiveOcclusionState(
    const FPrimitiveComponentId& id,
    bool previouslyOccluded) const {
  if (_currentOcclusionResults.occlusionResultsByView.size() == 0) {
    return TileOcclusionState::OcclusionUnavailable;
  }

  bool isOccluded = false;

  for (const SceneViewOcclusionResults& viewOcclusionResults : 
        _currentOcclusionResults.occlusionResultsByView) {
    const FPrimitiveOcclusionHistory* pHistory = 
        viewOcclusionResults.PrimitiveOcclusionHistorySet.Find(
          FPrimitiveOcclusionHistoryKey(id, 0));
    
    if (pHistory) {
      if (!pHistory->OcclusionStateWasDefiniteLastFrame) {
        // TODO: make sure this is handled correctly in BoundingVolumeComponent
        // e.g., if an object previously had definite occlusion results and now doesn't,
        // keep using old result.
        return TileOcclusionState::OcclusionUnavailable;
      }

      if (previouslyOccluded) {
        if (pHistory->LastPixelsPercentage > 0.01f) {
          return TileOcclusionState::NotOccluded;
        }
      } else if (!pHistory->WasOccludedLastFrame) {
        return TileOcclusionState::NotOccluded;
      }

      isOccluded = true;
    }
  }

  return isOccluded ? TileOcclusionState::Occluded : TileOcclusionState::NotOccluded;
}

void CesiumViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily) {}

void CesiumViewExtension::SetupView(
    FSceneViewFamily& InViewFamily,
    FSceneView& InView) {}

void CesiumViewExtension::BeginRenderViewFamily(
    FSceneViewFamily& InViewFamily) {
  if (!_occlusionResultsQueue.IsEmpty()) {
    _currentOcclusionResults = std::move(*_occlusionResultsQueue.Peek());
    _occlusionResultsQueue.Pop();
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
  if (_frameNumber_renderThread != InViewFamily.FrameNumber) {
    if (_frameNumber_renderThread != -1) {
      _occlusionResultsQueue.Enqueue(std::move(_currentAggregation_renderThread));
      _currentAggregation_renderThread = {};
    }

    _frameNumber_renderThread = InViewFamily.FrameNumber;
  }

  for (const FSceneView* pView : InViewFamily.Views) {
    const FSceneViewState* pViewState = pView->State->GetConcreteViewState();
    if (pViewState && pViewState->PrimitiveOcclusionHistorySet.Num()) {
      SceneViewOcclusionResults& occlusionResults =
          _currentAggregation_renderThread.occlusionResultsByView
              .emplace_back();
      // Do we actually need the view?
      occlusionResults.pView = pView;
      occlusionResults.PrimitiveOcclusionHistorySet =
          pViewState->PrimitiveOcclusionHistorySet;
    }
  }
}
