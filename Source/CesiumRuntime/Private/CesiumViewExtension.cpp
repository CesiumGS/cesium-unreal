
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

  return isOccluded ? TileOcclusionState::Occluded
                    : TileOcclusionState::NotOccluded;
}

void CesiumViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily) {}

void CesiumViewExtension::SetupView(
    FSceneViewFamily& InViewFamily,
    FSceneView& InView) {}

void CesiumViewExtension::BeginRenderViewFamily(
    FSceneViewFamily& InViewFamily) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::DequeueOcclusionResults)
  if (!_occlusionResultsQueue.IsEmpty()) {
    // Recycle the current occlusion results.
    for (SceneViewOcclusionResults& occlusionResults :
         _currentOcclusionResults.occlusionResultsByView) {
      occlusionResults.PrimitiveOcclusionHistorySet.Reset();
      _recycledOcclusionHistorySets.Enqueue(
          std::move(occlusionResults.PrimitiveOcclusionHistorySet));
    }
    _currentOcclusionResults = {};

    // Update occlusion results from the queue.
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
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EnqueueAggregatedOcclusion)
    if (_frameNumber_renderThread != -1) {
      _occlusionResultsQueue.Enqueue(
          std::move(_currentAggregation_renderThread));
      _currentAggregation_renderThread = {};
    }

    _frameNumber_renderThread = InViewFamily.FrameNumber;
  }

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::AggregateOcclusionForViewFamily)
  for (const FSceneView* pView : InViewFamily.Views) {
    const FSceneViewState* pViewState = pView->State->GetConcreteViewState();
    if (pViewState && pViewState->PrimitiveOcclusionHistorySet.Num()) {
      SceneViewOcclusionResults& occlusionResults =
          _currentAggregation_renderThread.occlusionResultsByView
              .emplace_back();
      // Do we actually need the view?
      occlusionResults.pView = pView;

      if (!_recycledOcclusionHistorySets.IsEmpty()) {
        // Recycle a previously allocated occlusion history set, if one is
        // available.
        occlusionResults.PrimitiveOcclusionHistorySet =
            std::move(*_recycledOcclusionHistorySets.Peek());
        _recycledOcclusionHistorySets.Pop();
        occlusionResults.PrimitiveOcclusionHistorySet.Append(
            pViewState->PrimitiveOcclusionHistorySet);
      } else {
        // If no previously-allocated set exists, just allocate a new one. It
        // will be recycled later.
        occlusionResults.PrimitiveOcclusionHistorySet =
            pViewState->PrimitiveOcclusionHistorySet;
      }
    }
  }
}
