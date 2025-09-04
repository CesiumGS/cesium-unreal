// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPolylineVertexFactory.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialDomain.h"
#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "RenderCommandFence.h"
#include "Runtime/Launch/Resources/Version.h"

void FCesiumPolylineIndexBuffer::InitRHI(FRHICommandListBase& RHICmdList) {
  // if (!bAttenuationSupported) {
  //   return;
  // }

  // This must be called from Rendering thread
  check(IsInRenderingThread());

  FRHIResourceCreateInfo CreateInfo(TEXT("FCesiumPolylineIndexBuffer"));

  // Each line segment of the polyline is represented as a quad that is
  // stretches from one point to the next.
  const uint32 NumIndices = NumLines * 4;
  const uint32 Size = NumIndices * sizeof(uint32);

  IndexBufferRHI = RHICmdList.CreateBuffer(
      Size,
      BUF_Static | BUF_IndexBuffer,
      sizeof(uint32),
      ERHIAccess::VertexOrIndexBuffer,
      CreateInfo);

  uint32* Data =
      (uint32*)RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);

  for (uint32 index = 0, bufferIndex = 0; bufferIndex < NumIndices;
       index += 4) {
    // Generate six indices per quad.
    Data[bufferIndex++] = index;
    Data[bufferIndex++] = index + 1;
    Data[bufferIndex++] = index + 2;
    Data[bufferIndex++] = index;
    Data[bufferIndex++] = index + 2;
    Data[bufferIndex++] = index + 3;
  }

  RHICmdList.UnlockBuffer(IndexBufferRHI);
}

class FCesiumPolylineVertexFactoryShaderParameters
    : public FVertexFactoryShaderParameters {

  DECLARE_TYPE_LAYOUT(FCesiumPolylineVertexFactoryShaderParameters, NonVirtual);

public:
  void Bind(const FShaderParameterMap& ParameterMap) {
    PositionBuffer.Bind(ParameterMap, TEXT("PositionBuffer"));
    PackedTangentsBuffer.Bind(ParameterMap, TEXT("PackedTangentsBuffer"));
    ColorBuffer.Bind(ParameterMap, TEXT("ColorBuffer"));
    TexCoordBuffer.Bind(ParameterMap, TEXT("TexCoordBuffer"));
    NumTexCoords.Bind(ParameterMap, TEXT("NumTexCoords"));
    NumPolylinePoints.Bind(ParameterMap, TEXT("NumPolylinePoints"));
    LineWidth.Bind(ParameterMap, TEXT("LineWidth"));
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
    FCesiumPolylineBatchElementUserData* UserData =
        (FCesiumPolylineBatchElementUserData*)BatchElement.UserData;
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
    if (NumPolylinePoints.IsBound()) {
      ShaderBindings.Add(NumPolylinePoints, UserData->NumPolylinePoints);
    }
    if (LineWidth.IsBound()) {
      ShaderBindings.Add(LineWidth, UserData->LineWidth);
    }
  }

private:
  LAYOUT_FIELD(FShaderResourceParameter, PositionBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, PackedTangentsBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, ColorBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, TexCoordBuffer);
  LAYOUT_FIELD(FShaderParameter, NumTexCoords);
  LAYOUT_FIELD(FShaderParameter, NumPolylinePoints);
  LAYOUT_FIELD(FShaderParameter, LineWidth);
};

/**
 * A dummy vertex buffer to bind when rendering polylines. This prevents
 * rendering pipeline errors that can occur with zero-stream input layouts.
 */
class FCesiumPolylineDummyVertexBuffer : public FVertexBuffer {
public:
  virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
};

void FCesiumPolylineDummyVertexBuffer::InitRHI(
    FRHICommandListBase& RHICmdList) {
  FRHIResourceCreateInfo CreateInfo(TEXT("FCesiumPolylineDummyVertexBuffer"));
  VertexBufferRHI = RHICmdList.CreateBuffer(
      sizeof(FVector3f) * 4,
      BUF_Static | BUF_VertexBuffer,
      0,
      ERHIAccess::VertexOrIndexBuffer,
      CreateInfo);
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

TGlobalResource<FCesiumPolylineDummyVertexBuffer>
    GCesiumPolylineDummyVertexBuffer;

FCesiumPolylineVertexFactory::FCesiumPolylineVertexFactory(
    ERHIFeatureLevel::Type InFeatureLevel,
    const FPositionVertexBuffer* PositionVertexBuffer)
    : FLocalVertexFactory(InFeatureLevel, "FCesiumPolylineVertexFactory") {}

bool FCesiumPolylineVertexFactory::ShouldCompilePermutation(
    const FVertexFactoryShaderPermutationParameters& Parameters) {
  if (!RHISupportsManualVertexFetch(Parameters.Platform)) {
    return false;
  }

  return Parameters.MaterialParameters.MaterialDomain == MD_Surface ||
         Parameters.MaterialParameters.bIsDefaultMaterial ||
         Parameters.MaterialParameters.bIsSpecialEngineMaterial;
}

void FCesiumPolylineVertexFactory::ModifyCompilationEnvironment(
    const FVertexFactoryShaderPermutationParameters& Parameters,
    FShaderCompilerEnvironment& OutEnvironment) {
  FLocalVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);

#if ENGINE_VERSION_5_5_OR_HIGHER
  OutEnvironment.SetDefine(TEXT("ENGINE_VERSION_5_5_OR_HIGHER"), TEXT("1"));
#endif
}

void FCesiumPolylineVertexFactory::InitRHI(FRHICommandListBase& RHICmdList) {
  FVertexDeclarationElementList Elements;
  Elements.Add(AccessStreamComponent(
      FVertexStreamComponent(
          &GCesiumPolylineDummyVertexBuffer,
          0,
          sizeof(FVector3f),
          VET_Float3),
      0));
  InitDeclaration(Elements);
}

void FCesiumPolylineVertexFactory::ReleaseRHI() {
  FVertexFactory::ReleaseRHI();
}

IMPLEMENT_TYPE_LAYOUT(FCesiumPolylineVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(
    FCesiumPolylineVertexFactory,
    SF_Vertex,
    FCesiumPolylineVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(
    FCesiumPolylineVertexFactory,
    "/Plugin/CesiumForUnreal/Private/CesiumPolylineVertexFactory.ush",
    EVertexFactoryFlags::UsedWithMaterials |
        EVertexFactoryFlags::SupportsDynamicLighting |
        EVertexFactoryFlags::SupportsPositionOnly);
