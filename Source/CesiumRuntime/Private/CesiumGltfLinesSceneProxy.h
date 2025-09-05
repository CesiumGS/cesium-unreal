// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "PrimitiveSceneProxy.h"

class UCesiumGltfLinesComponent;

class FCesiumGltfLinesSceneProxy final : public FPrimitiveSceneProxy {
private:
  // The original render data of the static mesh.
  const FStaticMeshRenderData* RenderData;
  int32_t NumLines;

public:
  SIZE_T GetTypeHash() const override;

  FCesiumGltfLinesSceneProxy(
      UCesiumGltfLinesComponent* InComponent,
      ERHIFeatureLevel::Type InFeatureLevel);

  virtual ~FCesiumGltfLinesSceneProxy();

protected:
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
  UMaterialInterface* Material;
  FMaterialRelevance MaterialRelevance;

  void CreateMesh(FMeshBatch& Mesh) const;
};
