
#pragma once

#include "Containers/Queue.h"
#include "Containers/Set.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "SceneTypes.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <cstdint>
#include <unordered_set>

class ACesium3DTileset;

class CesiumViewExtension : public FSceneViewExtensionBase {
private:
  // Occlusion results for a single view.
  struct SceneViewOcclusionResults {
    const FSceneView* pView = nullptr;
    TSet<FPrimitiveOcclusionHistory, FPrimitiveOcclusionHistoryKeyFuncs>
        PrimitiveOcclusionHistorySet{};
  };

  // A collection of occlusion results by view.
  struct AggregatedOcclusionUpdate {
    std::vector<SceneViewOcclusionResults> occlusionResultsByView{};
  };

  // The current collection of occlusion results for this frame.
  AggregatedOcclusionUpdate _currentAggregation_renderThread{};
  AggregatedOcclusionUpdate _currentOcclusionResults{};

  // A queue to pass occlusion results from the render thread to the game
  // thread.
  TQueue<AggregatedOcclusionUpdate, EQueueMode::Spsc> _occlusionResultsQueue;

  // A queue to recycle the previously-allocated occlusion history sets. The
  // game thread recycles the sets by moving them into the queue and sending
  // them back to the render thread.
  TQueue<
      TSet<FPrimitiveOcclusionHistory, FPrimitiveOcclusionHistoryKeyFuncs>,
      EQueueMode::Spsc>
      _recycledOcclusionHistorySets;

  // The last known frame number. This is used to determine when an occlusion
  // results aggregation is complete.
  int64_t _frameNumber_renderThread = -1;

public:
  CesiumViewExtension(const FAutoRegister& autoRegister);
  ~CesiumViewExtension();

  Cesium3DTilesSelection::TileOcclusionState getPrimitiveOcclusionState(
      const FPrimitiveComponentId& id,
      bool previouslyOccluded,
      float frameTimeCutoff) const;

  void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
  void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
  void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
  void PreRenderViewFamily_RenderThread(
      FRHICommandListImmediate& RHICmdList,
      FSceneViewFamily& InViewFamily) override;
  void PreRenderView_RenderThread(
      FRHICommandListImmediate& RHICmdList,
      FSceneView& InView) override;
  void PostRenderViewFamily_RenderThread(
      FRHICommandListImmediate& RHICmdList,
      FSceneViewFamily& InViewFamily) override;
};
