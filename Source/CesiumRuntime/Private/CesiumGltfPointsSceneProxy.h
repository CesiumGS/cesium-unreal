// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCompat.h"
#include "CesiumPointAttenuationVertexFactory.h"
#include "CesiumPointCloudShading.h"
#include "PrimitiveSceneProxy.h"
#include <glm/vec3.hpp>

class UCesiumGltfPointsComponent;

/**
 * @brief Settings from UCesiumGltfPointsComponent and its owner
 * ACesium3DTileset, passed via render thread to FCesiumGltfPointsSceneProxy so
 * it may render with point attenuation.
 */
struct FCesiumGltfPointsSceneProxyAttenuationData {
  FCesiumPointCloudShading tilesetPointCloudShading;
  double maximumScreenSpaceError;
  bool usesAdditiveRefinement;
  float geometricError;
  glm::vec3 dimensions;
  int32 diameter;

  /**
   * @brief Constructs an instance with default values.
   */
  FCesiumGltfPointsSceneProxyAttenuationData();

  /**
   * @brief Constructs an instance with values from the given
   * UCesiumGltfPointsComponent.
   */
  FCesiumGltfPointsSceneProxyAttenuationData(
      UCesiumGltfPointsComponent* pComponent);
};

class FCesiumGltfPointsSceneProxy final : public FPrimitiveSceneProxy {
public:
  SIZE_T GetTypeHash() const override;

  FCesiumGltfPointsSceneProxy(
      UCesiumGltfPointsComponent* InComponent,
      FSceneInterfaceWrapper InSceneInterfaceParams);

  virtual ~FCesiumGltfPointsSceneProxy();

protected:
  virtual void
  CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;

  virtual void DestroyRenderThreadResources() override;

  virtual void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector) const override;

  virtual FPrimitiveViewRelevance
  GetViewRelevance(const FSceneView* View) const override;

  virtual uint32 GetMemoryFootprint(void) const override;

public:
  /**
   * @brief Updates the attenuation data for this scene proxy.
   * @param InAttenuationData The new attenuation data to use.
   */
  void UpdateAttenuationData(
      const FCesiumGltfPointsSceneProxyAttenuationData& InAttenuationData);

private:
  void CreatePointAttenuationUserData(
      FMeshBatchElement& BatchElement,
      const FSceneView* View,
      FMeshElementCollector& Collector) const;

  void CreateMeshWithAttenuation(
      FMeshBatch& Mesh,
      const FSceneView* View,
      FMeshElementCollector& Collector) const;
  void CreateMesh(FMeshBatch& Mesh) const;

  /**
   * @brief Returns the geometric error to use for point attenuation.
   *
   * If the tile's geometric error is 0, then the value will be estimated based
   * on other tile factors.
   */
  float getGeometricError() const;

  /**
   * @brief The original render data of the static mesh.
   */
  const FStaticMeshRenderData* _pRenderData;

  /**
   * @brief The number of points in the mesh.
   */
  int32_t _numPoints;

  /**
   * @brief Whether or not the shader platform supports attenuation.
   */
  bool _attenuationSupported;

  /**
   * @brief Settings from the UCesiumGltfPointsComponent that owns this scene
   * proxy, as well as its ACesium3DTileset, that influence point attenuation.
   */
  FCesiumGltfPointsSceneProxyAttenuationData _attenuationData;

  FCesiumPointAttenuationVertexFactory _attenuationVertexFactory;
  FCesiumPointAttenuationIndexBuffer _attenuationIndexBuffer;
  UMaterialInterface* _pMaterial;
  FMaterialRelevance _materialRelevance;
};
