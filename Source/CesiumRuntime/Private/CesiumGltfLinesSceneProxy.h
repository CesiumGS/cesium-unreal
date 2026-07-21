// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCompat.h"
#include "CesiumLineStyleVertexFactory.h"
#include "CesiumVertexFactoryCommon.h"
#include "PrimitiveSceneProxy.h"
#include "StaticMeshResources.h"

class UCesiumGltfLinesComponent;

class FCesiumGltfLinesSceneProxy final : public FPrimitiveSceneProxy {
public:
  SIZE_T GetTypeHash() const override;

  FCesiumGltfLinesSceneProxy(
      UCesiumGltfLinesComponent* InComponent,
      FSceneInterfaceWrapper InSceneInterfaceParams);

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
  void createLineStyleUserData(
      FMeshBatchElement& BatchElement,
      const FSceneView* View,
      FMeshElementCollector& Collector) const;
  void createMeshWithLineWidth(
      FMeshBatch& Mesh,
      const FSceneView* View,
      FMeshElementCollector& Collector) const;
  void createMesh(FMeshBatch& Mesh) const;

  bool shouldRenderWithLineWidth() const;

  /**
   * @brief The original render data of the static mesh.
   */
  const FStaticMeshRenderData* _pRenderData;

  /**
   * @brief The number of lines in the mesh.
   */
  int32_t _numLines;

  /**
   * @brief The desired line width.
   */
  int32_t _lineWidth;

  /**
   * @brief The desired line pattern.
   */
  uint16_t _pattern;

  /**
   * @brief Whether or not the shader platform supports "Manual Vertex Fetch",
   * which is required for attenuation.
   */
  bool _manualVertexFetchSupported;

  FCesiumLineStyleVertexFactory _lineStyleVertexFactory;
  FCesiumQuadIndexBuffer _quadIndexBuffer;
  UMaterialInterface* _pMaterial;
  FMaterialRelevance _materialRelevance;
};
