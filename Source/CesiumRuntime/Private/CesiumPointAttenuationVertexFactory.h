// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "Engine/StaticMesh.h"
#include "LocalVertexFactory.h"
#include "RHIDefinitions.h"
#include "RHIResources.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneManagement.h"

#if ENGINE_VERSION_5_3_OR_HIGHER
#define INIT_RHI_SIGNATURE InitRHI(FRHICommandListBase& RHICmdList)
#else
#define INIT_RHI_SIGNATURE InitRHI()
#endif

/**
 * This generates the indices necessary for point attenuation in a
 * FCesiumGltfPointsComponent.
 */
class FCesiumPointAttenuationIndexBuffer : public FIndexBuffer {
public:
  FCesiumPointAttenuationIndexBuffer(
      const int32& NumPoints,
      const bool bAttenuationSupported)
      : NumPoints(NumPoints), bAttenuationSupported(bAttenuationSupported) {}

  virtual void INIT_RHI_SIGNATURE override;

private:
  // The number of points in the original point mesh. Not to be confused with
  // the number of vertices in the attenuated point mesh.
  const int32 NumPoints;
  const bool bAttenuationSupported;
};

/**
 * The parameters to be passed as UserData to the
 * shader.
 */
struct FCesiumPointAttenuationBatchElementUserData {
  FRHIShaderResourceView* PositionBuffer;
  FRHIShaderResourceView* PackedTangentsBuffer;
  FRHIShaderResourceView* ColorBuffer;
  FRHIShaderResourceView* TexCoordBuffer;
  uint32 NumTexCoords;
  uint32 bHasPointColors;
  FVector3f AttenuationParameters;
};

class FCesiumPointAttenuationBatchElementUserDataWrapper
    : public FOneFrameResource {
public:
  FCesiumPointAttenuationBatchElementUserData Data;
};

class FCesiumPointAttenuationVertexFactory : public FLocalVertexFactory {

  DECLARE_VERTEX_FACTORY_TYPE(FCesiumPointAttenuationVertexFactory);

public:
  // Sets default values for this component's properties
  FCesiumPointAttenuationVertexFactory(
      ERHIFeatureLevel::Type InFeatureLevel,
      const FPositionVertexBuffer* PositionVertexBuffer);

  static bool ShouldCompilePermutation(
      const FVertexFactoryShaderPermutationParameters& Parameters);

private:
  virtual void INIT_RHI_SIGNATURE override;
  virtual void ReleaseRHI() override;
};
