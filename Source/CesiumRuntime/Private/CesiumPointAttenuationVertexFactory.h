// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Engine/StaticMesh.h"
#include "LocalVertexFactory.h"

/**
 * This generates the indices necessary for point attenuation in a
 * FCesiumGltfPointsComponent.
 */
class FCesiumPointAttenuationIndexBuffer : public FIndexBuffer {
public:
  FCesiumPointAttenuationIndexBuffer(const int32& NumPoints)
      : NumPoints(NumPoints) {}
  virtual void InitRHI() override;

private:
  // The number of points in the original point mesh. Not to be confused with
  // the number of vertices in the attenuated point mesh.
  const int32 NumPoints;
};

/**
 * The parameters to be passed as UserData to the
 * shader.
 */
struct FCesiumPointAttenuationBatchElementUserData {
  FRHIShaderResourceView* PositionBuffer;
  FVector AttenuationParameters;
  FVector4 ConstantColor;
};

class FCesiumPointAttenuationBatchElementUserDataWrapper : public FOneFrameResource {
public:
  FCesiumPointAttenuationBatchElementUserData Data;
};

class FCesiumPointAttenuationVertexFactory : public FLocalVertexFactory {

  DECLARE_VERTEX_FACTORY_TYPE(FCesiumGltfPointsVertexFactory);

public:
  // Sets default values for this component's properties
  FCesiumPointAttenuationVertexFactory(
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
