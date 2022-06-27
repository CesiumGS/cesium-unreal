
#include "CesiumViewExtension.h"

#include "Cesium3DTileset.h"

CesiumViewExtension::CesiumViewExtension(const FAutoRegister& autoRegister)
    : FSceneViewExtensionBase(autoRegister) {}

CesiumViewExtension::~CesiumViewExtension() {}

void CesiumViewExtension::RegisterTileset(ACesium3DTileset* pTileset) {
  this->_registeredTilesets.insert(pTileset);
}

void CesiumViewExtension::UnregisterTileset(ACesium3DTileset* pTileset) {
  this->_registeredTilesets.erase(pTileset);
}

void CesiumViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily) {}

void CesiumViewExtension::SetupView(
    FSceneViewFamily& InViewFamily,
    FSceneView& InView) {}

void CesiumViewExtension::BeginRenderViewFamily(
    FSceneViewFamily& InViewFamily) {
  for (ACesium3DTileset* pTileset : this->_registeredTilesets) {
    pTileset->UpdateFromView(InViewFamily);
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
    FSceneViewFamily& InViewFamily) {}
