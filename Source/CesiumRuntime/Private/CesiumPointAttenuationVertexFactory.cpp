// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPointAttenuationVertexFactory.h"

#include "CesiumCommon.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialDomain.h"
#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "RenderCommandFence.h"
#include "Runtime/Launch/Resources/Version.h"

namespace {
FBufferRHIRef CreatePointAttenuationBuffer(
    FRHICommandListBase& RHICmdList,
    const TCHAR* Name,
    int32 Size,
    int32 Stride,
    EBufferUsageFlags Flags) {
#if ENGINE_VERSION_5_6_OR_HIGHER
  FRHIBufferCreateDesc CreateDesc(Name, Size, Stride, Flags);
  CreateDesc.SetInitialState(ERHIAccess::VertexOrIndexBuffer);
  return RHICmdList.CreateBuffer(CreateDesc);
#else
  FRHIResourceCreateInfo CreateInfo(Name);
  return RHICmdList.CreateBuffer(
      Size,
      Flags,
      Stride,
      ERHIAccess::VertexOrIndexBuffer,
      CreateInfo);
#endif
}
} // namespace

void FCesiumPointAttenuationIndexBuffer::InitRHI(
    FRHICommandListBase& RHICmdList) {
  if (!bAttenuationSupported) {
    return;
  }

  // This must be called from Rendering thread
  check(IsInRenderingThread());

  const uint32 NumIndices = NumPoints * 6;
  const uint32 Size = NumIndices * sizeof(uint32);

  IndexBufferRHI = CreatePointAttenuationBuffer(
      RHICmdList,
      TEXT("FCesiumPointAttenuationIndexBuffer"),
      Size,
      sizeof(uint32),
      BUF_Static | BUF_IndexBuffer);

  uint32* Data =
      (uint32*)RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);

  for (uint32 index = 0, bufferIndex = 0; bufferIndex < NumIndices;
       index += 4) {
    // Generate six indices per quad, each representing an attenuated point in
    // the point cloud.
    Data[bufferIndex++] = index;
    Data[bufferIndex++] = index + 1;
    Data[bufferIndex++] = index + 2;
    Data[bufferIndex++] = index;
    Data[bufferIndex++] = index + 2;
    Data[bufferIndex++] = index + 3;
  }

  RHICmdList.UnlockBuffer(IndexBufferRHI);
}

class FCesiumPointAttenuationVertexFactoryShaderParameters
    : public FVertexFactoryShaderParameters {

  DECLARE_TYPE_LAYOUT(
      FCesiumPointAttenuationVertexFactoryShaderParameters,
      NonVirtual);

public:
  void Bind(const FShaderParameterMap& ParameterMap) {
    PositionBuffer.Bind(ParameterMap, TEXT("PositionBuffer"));
    PackedTangentsBuffer.Bind(ParameterMap, TEXT("PackedTangentsBuffer"));
    ColorBuffer.Bind(ParameterMap, TEXT("ColorBuffer"));
    TexCoordBuffer.Bind(ParameterMap, TEXT("TexCoordBuffer"));
    NumTexCoords.Bind(ParameterMap, TEXT("NumTexCoords"));
    bHasPointColors.Bind(ParameterMap, TEXT("bHasPointColors"));
    AttenuationParameters.Bind(ParameterMap, TEXT("AttenuationParameters"));
  }

  void GetElementShaderBindings(
      const FSceneInterface* Scene,
      const FSceneView* View,
      const FMeshMaterialShader* Shader,
      const EVertexInputStreamType InputStreamType,
      ERHIFeatureLevel::Type FeatureLevel,
      const FVertexFactory* VertexFactory,
      const FMeshBatchElement& BatchElement,
      FMeshDrawSingleShaderBindings& ShaderBindings,
      FVertexInputStreamArray& VertexStreams) const {
    FCesiumPointAttenuationBatchElementUserData* UserData =
        (FCesiumPointAttenuationBatchElementUserData*)BatchElement.UserData;
    if (UserData->PositionBuffer && PositionBuffer.IsBound()) {
      ShaderBindings.Add(PositionBuffer, UserData->PositionBuffer);
    }
    if (UserData->PackedTangentsBuffer && PackedTangentsBuffer.IsBound()) {
      ShaderBindings.Add(PackedTangentsBuffer, UserData->PackedTangentsBuffer);
    }
    if (UserData->ColorBuffer && ColorBuffer.IsBound()) {
      ShaderBindings.Add(ColorBuffer, UserData->ColorBuffer);
    }
    if (UserData->TexCoordBuffer && TexCoordBuffer.IsBound()) {
      ShaderBindings.Add(TexCoordBuffer, UserData->TexCoordBuffer);
    }
    if (NumTexCoords.IsBound()) {
      ShaderBindings.Add(NumTexCoords, UserData->NumTexCoords);
    }
    if (bHasPointColors.IsBound()) {
      ShaderBindings.Add(bHasPointColors, UserData->bHasPointColors);
    }
    if (AttenuationParameters.IsBound()) {
      ShaderBindings.Add(
          AttenuationParameters,
          UserData->AttenuationParameters);
    }
  }

private:
  LAYOUT_FIELD(FShaderResourceParameter, PositionBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, PackedTangentsBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, ColorBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, TexCoordBuffer);
  LAYOUT_FIELD(FShaderParameter, NumTexCoords);
  LAYOUT_FIELD(FShaderParameter, bHasPointColors);
  LAYOUT_FIELD(FShaderParameter, AttenuationParameters);
};

/**
 * A dummy vertex buffer to bind when rendering attenuated point clouds. This
 * prevents rendering pipeline errors that can occur with zero-stream input
 * layouts.
 */
class FCesiumPointAttenuationDummyVertexBuffer : public FVertexBuffer {
public:
  virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
};

void FCesiumPointAttenuationDummyVertexBuffer::InitRHI(
    FRHICommandListBase& RHICmdList) {
  VertexBufferRHI = CreatePointAttenuationBuffer(
      RHICmdList,
      TEXT("FCesiumPointAttenuationDummyVertexBuffer"),
      sizeof(FVector3f) * 4,
      0,
      BUF_Static | BUF_VertexBuffer);
  FVector3f* DummyContents = (FVector3f*)RHICmdList.LockBuffer(
      VertexBufferRHI,
      0,
      sizeof(FVector3f) * 4,
      RLM_WriteOnly);
  DummyContents[0] = FVector3f(0.0f, 0.0f, 0.0f);
  DummyContents[1] = FVector3f(1.0f, 0.0f, 0.0f);
  DummyContents[2] = FVector3f(0.0f, 1.0f, 0.0f);
  DummyContents[3] = FVector3f(1.0f, 1.0f, 0.0f);
  RHICmdList.UnlockBuffer(VertexBufferRHI);
}

TGlobalResource<FCesiumPointAttenuationDummyVertexBuffer>
    GCesiumPointAttenuationDummyVertexBuffer;

FCesiumPointAttenuationVertexFactory::FCesiumPointAttenuationVertexFactory(
    ERHIFeatureLevel::Type InFeatureLevel,
    const FPositionVertexBuffer* PositionVertexBuffer)
    : FLocalVertexFactory(
          InFeatureLevel,
          "FCesiumPointAttenuationVertexFactory") {}

bool FCesiumPointAttenuationVertexFactory::ShouldCompilePermutation(
    const FVertexFactoryShaderPermutationParameters& Parameters) {
  if (!RHISupportsManualVertexFetch(Parameters.Platform)) {
    return false;
  }

  return Parameters.MaterialParameters.MaterialDomain == MD_Surface ||
         Parameters.MaterialParameters.bIsDefaultMaterial ||
         Parameters.MaterialParameters.bIsSpecialEngineMaterial;
}

void FCesiumPointAttenuationVertexFactory::ModifyCompilationEnvironment(
    const FVertexFactoryShaderPermutationParameters& Parameters,
    FShaderCompilerEnvironment& OutEnvironment) {
  FLocalVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

void FCesiumPointAttenuationVertexFactory::InitRHI(
    FRHICommandListBase& RHICmdList) {
  FVertexDeclarationElementList Elements;
  Elements.Add(AccessStreamComponent(
      FVertexStreamComponent(
          &GCesiumPointAttenuationDummyVertexBuffer,
          0,
          sizeof(FVector3f),
          VET_Float3),
      0));
  InitDeclaration(Elements);
}

void FCesiumPointAttenuationVertexFactory::ReleaseRHI() {
  FVertexFactory::ReleaseRHI();
}

IMPLEMENT_TYPE_LAYOUT(FCesiumPointAttenuationVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(
    FCesiumPointAttenuationVertexFactory,
    SF_Vertex,
    FCesiumPointAttenuationVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(
    FCesiumPointAttenuationVertexFactory,
    "/Plugin/CesiumForUnreal/Private/CesiumPointAttenuationVertexFactory.ush",
    EVertexFactoryFlags::UsedWithMaterials |
        EVertexFactoryFlags::SupportsDynamicLighting |
        EVertexFactoryFlags::SupportsPositionOnly);
