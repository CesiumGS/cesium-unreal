// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "LocalVertexFactory.h"
#include "Engine/StaticMesh.h"

class FCesiumGltfPointsVertexFactory : public FLocalVertexFactory {

  DECLARE_VERTEX_FACTORY_TYPE(FCesiumGltfPointsVertexFactory);

public:
  // Sets default values for this component's properties
  FCesiumGltfPointsVertexFactory(
      ERHIFeatureLevel::Type InFeatureLevel,
      const FStaticMeshVertexBuffers& InStaticMeshVertexBuffers);

  static bool ShouldCompilePermutation(
      const FVertexFactoryShaderPermutationParameters& Parameters);

private:
  FVertexBuffer VertexBuffer;
  const FStaticMeshVertexBuffers& StaticMeshVertexBuffers;

  virtual void InitRHI() override;
  virtual void ReleaseRHI() override;
};
