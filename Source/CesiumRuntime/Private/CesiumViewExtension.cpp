// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumViewExtension.h"

#include "Cesium3DTileset.h"
#include "CesiumCommon.h"
#include "Runtime/Launch/Resources/Version.h"

using namespace Cesium3DTilesSelection;

CesiumViewExtension::CesiumViewExtension(const FAutoRegister& autoRegister)
    : FSceneViewExtensionBase(autoRegister) {}

CesiumViewExtension::~CesiumViewExtension() = default;

TileOcclusionState CesiumViewExtension::getPrimitiveOcclusionState(
    const FPrimitiveComponentId& id,
    bool previouslyOccluded,
    float frameTimeCutoff) const {
  if (_currentOcclusionResults.occlusionResultsByView.size() == 0) {
    return TileOcclusionState::OcclusionUnavailable;
  }

  bool isOccluded = false;
  bool historyMissing = false;

  for (const SceneViewOcclusionResults& viewOcclusionResults :
       _currentOcclusionResults.occlusionResultsByView) {
    const PrimitiveOcclusionResult* pOcclusionResult =
        viewOcclusionResults.PrimitiveOcclusionResults.Find(id);

    if (pOcclusionResult &&
        pOcclusionResult->LastConsideredTime >= frameTimeCutoff) {
      if (!pOcclusionResult->OcclusionStateWasDefiniteLastFrame) {
        return TileOcclusionState::OcclusionUnavailable;
      }

      if (previouslyOccluded) {
        if (pOcclusionResult->LastPixelsPercentage > 0.01f) {
          return TileOcclusionState::NotOccluded;
        }
      } else if (!pOcclusionResult->WasOccludedLastFrame) {
        return TileOcclusionState::NotOccluded;
      }

      isOccluded = true;
    } else {
      historyMissing = true;
    }
  }

  if (historyMissing) {
    return TileOcclusionState::OcclusionUnavailable;
  } else if (isOccluded) {
    return TileOcclusionState::Occluded;
  } else {
    return TileOcclusionState::NotOccluded;
  }
}

void CesiumViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily) {}

void CesiumViewExtension::SetupView(
    FSceneViewFamily& InViewFamily,
    FSceneView& InView) {}

void CesiumViewExtension::BeginRenderViewFamily(
    FSceneViewFamily& InViewFamily) {
  if (!this->_isEnabled)
    return;

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::DequeueOcclusionResults)
  if (!_occlusionResultsQueue.IsEmpty()) {
    // Recycle the current occlusion results.
    for (SceneViewOcclusionResults& occlusionResults :
         _currentOcclusionResults.occlusionResultsByView) {
      occlusionResults.PrimitiveOcclusionResults.Reset();
      _recycledOcclusionResultSets.Enqueue(
          std::move(occlusionResults.PrimitiveOcclusionResults));
    }
    _currentOcclusionResults = {};

    // Update occlusion results from the queue.
    _currentOcclusionResults = std::move(*_occlusionResultsQueue.Peek());
    _occlusionResultsQueue.Pop();
  }
}

namespace {

const TSet<FPrimitiveOcclusionHistory, FPrimitiveOcclusionHistoryKeyFuncs>&
getOcclusionHistorySet(const FSceneViewState* pViewState) {
#if ENGINE_VERSION_5_3_OR_HIGHER
  return pViewState->Occlusion.PrimitiveOcclusionHistorySet;
#else
  return pViewState->PrimitiveOcclusionHistorySet;
#endif
}

} // namespace

void CesiumViewExtension::PostRenderViewFamily_RenderThread(
    FRDGBuilder& GraphBuilder,
    FSceneViewFamily& InViewFamily) {
  if (!this->_isEnabled)
    return;

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
    if (pView == nullptr || pView->State == nullptr)
      continue;

    const FSceneViewState* pViewState = pView->State->GetConcreteViewState();
    if (pViewState && getOcclusionHistorySet(pViewState).Num()) {
      SceneViewOcclusionResults& occlusionResults =
          _currentAggregation_renderThread.occlusionResultsByView
              .emplace_back();
      // Do we actually need the view?
      occlusionResults.pView = pView;

      if (!_recycledOcclusionResultSets.IsEmpty()) {
        // Recycle a previously allocated occlusion history set, if one is
        // available.
        occlusionResults.PrimitiveOcclusionResults =
            std::move(*_recycledOcclusionResultSets.Peek());
        _recycledOcclusionResultSets.Pop();
      } else {
        // If no previously-allocated set exists, just allocate a new one. It
        // will be recycled later.
      }

      occlusionResults.PrimitiveOcclusionResults.Reserve(
          getOcclusionHistorySet(pViewState).Num());
      for (const auto& element : getOcclusionHistorySet(pViewState)) {
        occlusionResults.PrimitiveOcclusionResults.Emplace(element);
      }

      // Unreal will not execute occlusion queries that get frustum culled in a
      // particular view, leaving the occlusion results indefinite. And by just
      // looking at the PrimitiveOcclusionHistorySet, we can't distinguish
      // occlusion queries that haven't completed "yet" from occlusion queries
      // that were culled. So here we detect primitives that have been
      // conclusively proven to be not visible (outside the view frustum) and
      // also mark them definitely occluded.
      FScene* pScene = InViewFamily.Scene->GetRenderScene();
      if (pView->bIsViewInfo && pScene != nullptr) {
        const FViewInfo* pViewInfo = static_cast<const FViewInfo*>(pView);
        const FSceneBitArray& visibility = pViewInfo->PrimitiveVisibilityMap;
        auto& occlusion = occlusionResults.PrimitiveOcclusionResults;

        const uint32 PrimitiveCount = pScene->Primitives.Num();
        for (uint32 i = 0; i < PrimitiveCount; ++i) {
          FPrimitiveSceneInfo* pSceneInfo = pScene->Primitives[i];
          if (pSceneInfo == nullptr)
            continue;

          bool setOcclusionState = false;
          bool isOccluded = false;

          // Unreal will never compute occlusion for primitives that are
          // selected in the Editor. So treat these as unoccluded.
#if WITH_EDITOR
          if (GIsEditor) {
            if (pScene->PrimitivesSelected[i]) {
              setOcclusionState = true;
              isOccluded = false;
            }
          }
#endif

          // If this primitive is not visible at all (and also not selected!),
          // treat is as occluded.
          if (!setOcclusionState && !visibility[i]) {
            setOcclusionState = true;
            isOccluded = true;
          }

          if (setOcclusionState) {
            const PrimitiveOcclusionResult* pOcclusionResult =
                occlusion.Find(pSceneInfo->PrimitiveComponentId);
            if (!pOcclusionResult || pOcclusionResult->LastConsideredTime <
                                         pViewState->LastRenderTime) {
              // No valid occlusion history for this culled primitive, so create
              // it.
              occlusion.Emplace(PrimitiveOcclusionResult(
                  pSceneInfo->PrimitiveComponentId,
                  pViewState->LastRenderTime,
                  isOccluded ? 0.0f : 100.0f,
                  true,
                  isOccluded));
            }
          }
        }
      }
    }
  }
}

void CesiumViewExtension::SetEnabled(bool enabled) {
  this->_isEnabled = enabled;
}
