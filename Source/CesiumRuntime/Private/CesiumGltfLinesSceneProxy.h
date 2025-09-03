// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPolylineVertexFactory.h"

#include "PrimitiveSceneProxy.h"

class UCesiumGltfLinesComponent;

class FCesiumGltfLinesSceneProxy final : public FPrimitiveSceneProxy {
private:
  // The original render data of the static mesh.
  const FStaticMeshRenderData* RenderData;
  int32_t NumLines;
  float LineWidth;

public:
  SIZE_T GetTypeHash() const override;

  FCesiumGltfLinesSceneProxy(
      UCesiumGltfLinesComponent* InComponent,
      ERHIFeatureLevel::Type InFeatureLevel);

  virtual ~FCesiumGltfLinesSceneProxy();

protected:
  virtual void
  CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;

  virtual void DestroyRenderThreadResources() override;

  virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;

  virtual void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector) const override;

  virtual FPrimitiveViewRelevance
  GetViewRelevance(const FSceneView* View) const override;

  virtual uint32 GetMemoryFootprint(void) const override;

private:
  // The vertex factory and index buffer for thick line rendering.
  FCesiumPolylineVertexFactory PolylineVertexFactory;
  FCesiumPolylineIndexBuffer PolylineIndexBuffer;

  UMaterialInterface* Material;
  FMaterialRelevance MaterialRelevance;

  void CreatePolylineUserData(
      FMeshBatchElement& BatchElement,
      const FSceneView* View,
      FMeshElementCollector& Collector) const;

  void CreatePolylineMesh(
      FMeshBatch& Mesh,
      const FSceneView* View,
      FMeshElementCollector& Collector) const;

  void CreateMesh(FMeshBatch& Mesh) const;
};
