
#pragma once

#include "SceneTypes.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "Containers/Queue.h"
#include "Containers/Set.h"
#include "SceneTypes.h"
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <unordered_set>
#include <cstdint>

class ACesium3DTileset;

class CesiumViewExtension : public FSceneViewExtensionBase {
private:
  struct SceneViewOcclusionResults {
    const FSceneView* pView = nullptr;
    TSet<FPrimitiveOcclusionHistory,FPrimitiveOcclusionHistoryKeyFuncs> PrimitiveOcclusionHistorySet{};
  };

  struct AggregatedOcclusionUpdate {
    std::vector<SceneViewOcclusionResults> occlusionResultsByView{};
  };

  AggregatedOcclusionUpdate _currentAggregation_renderThread{};
  AggregatedOcclusionUpdate _currentOcclusionResults{};

  TQueue<AggregatedOcclusionUpdate, EQueueMode::Spsc> _occlusionResultsQueue;
  int64_t _frameNumber_renderThread = -1;

public:
  CesiumViewExtension(const FAutoRegister& autoRegister);
  ~CesiumViewExtension();

  Cesium3DTilesSelection::TileOcclusionState getPrimitiveOcclusionState(
      const FPrimitiveComponentId& id, bool previouslyOccluded) const;

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
