// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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
class UCesiumGltfPrimitiveComponent;

/**
 * @brief A view extension that enables Cesium for Unreal to hook into the
 * Unreal rendering passes. This enables occlusion culling.
 */
class CesiumViewExtension : public FSceneViewExtensionBase {
public:
  CesiumViewExtension(const FAutoRegister& autoRegister);
  ~CesiumViewExtension();

  /**
   * Called on game thread when creating the view family.
   */
  void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}

  /**
   * Called on game thread when creating the view.
   */
  void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}

  /**
   * Called on game thread after the scene renderers have been created
   */
  virtual void PostCreateSceneRenderer(
      const FSceneViewFamily& InViewFamily,
      ISceneRenderer* Renderer) override;

  /**
   * Called on game thread when view family is about to be rendered.
   */
  void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

  /**
   * Called on render thread at the start of rendering, for each view, after
   * PreRenderViewFamily_RenderThread call.
   */
  void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
      override;

  /**
   * Called right after Base Pass rendering finished when using the deferred
   * renderer.
   */
  virtual void PostRenderBasePassDeferred_RenderThread(
      FRDGBuilder& GraphBuilder,
      FSceneView& InView,
      const FRenderTargetBindingSlots& RenderTargets,
      TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
      override;

  /**
   * Allows to render content after the 3D content scene, useful for debugging
   */
  void PostRenderViewFamily_RenderThread(
      FRDGBuilder& GraphBuilder,
      FSceneViewFamily& InViewFamily) override;

  /**
   * This will be called at the beginning of post processing to make sure that
   * each view extension gets a chance to subscribe to an after pass event.
   *  - The pass MUST write to the override output texture if it is active (this
   * occurs when the pass is the last in the post processing chain writing to
   * the back buffer). For performance reasons it is recommended to only
   * subscribe to a pass when the pass will produce a GPU resource.
   */
  virtual void SubscribeToPostProcessingPass(
      EPostProcessingPass Pass,
      const FSceneView& InView,
      FAfterPassCallbackDelegateArray& InOutPassCallbacks,
      bool bIsPassEnabled) override;

  void SetEnabled(bool enabled);

  Cesium3DTilesSelection::TileOcclusionState getPrimitiveOcclusionState(
      const FPrimitiveComponentId& id,
      bool previouslyOccluded,
      float frameTimeCutoff) const;

  void registerComponentWithEdges(UCesiumGltfPrimitiveComponent* pComponent);
  void unregisterComponentWithEdges(UCesiumGltfPrimitiveComponent* pComponent);

private:
#pragma region Occlusion Culling
  /**
   * Occlusion results for a single view.
   */
  struct PrimitiveOcclusionResult {
    PrimitiveOcclusionResult(
        const FPrimitiveComponentId primitiveId,
        float lastConsideredTime,
        float lastPixelsPercentage,
        bool occlusionStateWasDefiniteLastFrame,
        bool wasOccludedLastFrame)
        : PrimitiveId(primitiveId),
          LastConsideredTime(lastConsideredTime),
          LastPixelsPercentage(lastPixelsPercentage),
          OcclusionStateWasDefiniteLastFrame(
              occlusionStateWasDefiniteLastFrame),
          WasOccludedLastFrame(wasOccludedLastFrame) {}

    PrimitiveOcclusionResult(const FPrimitiveOcclusionHistory& renderer)
        : PrimitiveId(renderer.PrimitiveId),
          LastConsideredTime(renderer.LastConsideredTime),
          LastPixelsPercentage(renderer.LastPixelsPercentage),
          OcclusionStateWasDefiniteLastFrame(
              renderer.OcclusionStateWasDefiniteLastFrame),
          WasOccludedLastFrame(renderer.WasOccludedLastFrame) {}

    FPrimitiveComponentId PrimitiveId;
    float LastConsideredTime;
    float LastPixelsPercentage;
    bool OcclusionStateWasDefiniteLastFrame;
    bool WasOccludedLastFrame;
  };

  /**
   * Defines how PrimitiveOcclusionResult is stored in a TSet
   */
  struct PrimitiveOcclusionResultKeyFuncs
      : BaseKeyFuncs<PrimitiveOcclusionResult, FPrimitiveComponentId> {
    typedef FPrimitiveComponentId KeyInitType;

    static KeyInitType GetSetKey(const PrimitiveOcclusionResult& Element) {
      return Element.PrimitiveId;
    }

    static bool Matches(KeyInitType A, KeyInitType B) { return A == B; }

    static uint32 GetKeyHash(KeyInitType Key) {
      return GetTypeHash(Key.PrimIDValue);
    }
  };

  /**
   *The occlusion results for a single view.
   */
  struct SceneViewOcclusionResults {
    const FSceneView* pView = nullptr;
    TSet<PrimitiveOcclusionResult, PrimitiveOcclusionResultKeyFuncs>
        PrimitiveOcclusionResults{};
  };

  /**
   *A collection of occlusion results by view.
   */
  struct AggregatedOcclusionUpdate {
    std::vector<SceneViewOcclusionResults> occlusionResultsByView{};
  };

  // The current collection of occlusion results for this frame.
  AggregatedOcclusionUpdate _currentAggregation_renderThread{};
  AggregatedOcclusionUpdate _currentOcclusionResults{};

  // A queue to pass occlusion results from the render thread to the game
  // thread.
  TQueue<AggregatedOcclusionUpdate, EQueueMode::Spsc> _occlusionResultsQueue;

  // A queue to recycle the previously-allocated occlusion result sets. The
  // game thread recycles the sets by moving them into the queue and sending
  // them back to the render thread.
  TQueue<
      TSet<PrimitiveOcclusionResult, PrimitiveOcclusionResultKeyFuncs>,
      EQueueMode::Spsc>
      _recycledOcclusionResultSets;

  // The last known frame number. This is used to determine when an occlusion
  // results aggregation is complete.
  int64_t _frameNumber_renderThread = -1;
#pragma endregion

#pragma region EXT_mesh_primitive_edge_visibility

  TSet<UCesiumGltfPrimitiveComponent*> _componentsWithEdgeVisibility;
  TSet<FPrimitiveComponentId> _componentIds;

#pragma endregion

  std::atomic<bool> _isEnabled = false;
};
