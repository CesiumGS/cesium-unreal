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
 * @brief Parameters to pass as UserData to the CesiumLineStyleVertexFactory
 * shader.
 */
struct FCesiumLineStyleBatchElementUserData {
  FRHIShaderResourceView* IndexBuffer;
  FRHIShaderResourceView* PositionBuffer;
  FRHIShaderResourceView* PackedTangentsBuffer;
  FRHIShaderResourceView* ColorBuffer;
  FRHIShaderResourceView* TexCoordBuffer;
  uint32 NumTexCoords;
  float LineWidth;
  uint16 Pattern;
};

class FCesiumLineStyleBatchElementUserDataWrapper : public FOneFrameResource {
public:
  FCesiumLineStyleBatchElementUserData Data;
};

class FCesiumLineStyleVertexFactory : public FLocalVertexFactory {

  DECLARE_VERTEX_FACTORY_TYPE(FCesiumLineStyleVertexFactory);

public:
  // Sets default values for this component's properties
  FCesiumLineStyleVertexFactory(
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
