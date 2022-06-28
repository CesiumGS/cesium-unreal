
#pragma once

#include "SceneTypes.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include <unordered_set>

class ACesium3DTileset;

class CesiumViewExtension : public FSceneViewExtensionBase {
private:
  std::unordered_set<ACesium3DTileset*> _registeredTilesets;

public:
  CesiumViewExtension(const FAutoRegister& autoRegister);
  ~CesiumViewExtension();

  void RegisterTileset(ACesium3DTileset* pTileset);
  void UnregisterTileset(ACesium3DTileset* pTileset);

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
