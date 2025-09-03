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

/**
 * This generates the indices necessary for thick polyline rendering in a
 * FCesiumGltfLinesComponent.
 */
class FCesiumPolylineIndexBuffer : public FIndexBuffer {
public:
  FCesiumPolylineIndexBuffer(
      const int32& NumLines,
      const bool bAttenuationSupported)
      : NumLines(NumLines) {}

  virtual void InitRHI(FRHICommandListBase& RHICmdList) override;

private:
  // The number of lines in the original line mesh. Not to be confused with
  // the number of vertices in the expanded polyline mesh.
  const int32 NumLines;
};

/**
 * The parameters to be passed as UserData to the
 * shader.
 */
struct FCesiumPolylineBatchElementUserData {
  FRHIShaderResourceView* PositionBuffer;
  FRHIShaderResourceView* PackedTangentsBuffer;
  FRHIShaderResourceView* ColorBuffer;
  FRHIShaderResourceView* TexCoordBuffer;
  uint32 NumTexCoords;
  uint32 NumPolylinePoints;
  float LineWidth;
};

class FCesiumPolylineBatchElementUserDataWrapper : public FOneFrameResource {
public:
  FCesiumPolylineBatchElementUserData Data;
};

class FCesiumPolylineVertexFactory : public FLocalVertexFactory {

  DECLARE_VERTEX_FACTORY_TYPE(FCesiumPolylineVertexFactory);

public:
  // Sets default values for this component's properties
  FCesiumPolylineVertexFactory(
      ERHIFeatureLevel::Type InFeatureLevel,
      const FPositionVertexBuffer* PositionVertexBuffer);

  static bool ShouldCompilePermutation(
      const FVertexFactoryShaderPermutationParameters& Parameters);

  static void ModifyCompilationEnvironment(
      const FVertexFactoryShaderPermutationParameters& Parameters,
      FShaderCompilerEnvironment& OutEnvironment);

private:
  virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
  virtual void ReleaseRHI() override;
};
