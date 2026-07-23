// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumLineStyleVertexFactory.h"

#include "CesiumVertexFactoryCommon.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialDomain.h"
#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "RenderCommandFence.h"
#include "Runtime/Launch/Resources/Version.h"

class FCesiumLineStyleVertexFactoryShaderParameters
    : public FVertexFactoryShaderParameters {

  DECLARE_TYPE_LAYOUT(
      FCesiumLineStyleVertexFactoryShaderParameters,
      NonVirtual);

public:
  void Bind(const FShaderParameterMap& ParameterMap) {
    IndexBuffer.Bind(ParameterMap, TEXT("IndexBuffer"));
    PositionBuffer.Bind(ParameterMap, TEXT("PositionBuffer"));
    PackedTangentsBuffer.Bind(ParameterMap, TEXT("PackedTangentsBuffer"));
    ColorBuffer.Bind(ParameterMap, TEXT("ColorBuffer"));
    TexCoordBuffer.Bind(ParameterMap, TEXT("TexCoordBuffer"));
    NumTexCoords.Bind(ParameterMap, TEXT("NumTexCoords"));
    bHasVertexColors.Bind(ParameterMap, TEXT("bHasVertexColors"));
    LineWidth.Bind(ParameterMap, TEXT("LineWidth"));
    Pattern.Bind(ParameterMap, TEXT("Pattern"));
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
    FCesiumLineStyleBatchElementUserData* pUserData =
        (FCesiumLineStyleBatchElementUserData*)BatchElement.UserData;
    if (pUserData->IndexBuffer && IndexBuffer.IsBound()) {
      ShaderBindings.Add(IndexBuffer, pUserData->IndexBuffer);
    }
    if (pUserData->PositionBuffer && PositionBuffer.IsBound()) {
      ShaderBindings.Add(PositionBuffer, pUserData->PositionBuffer);
    }
    if (pUserData->PackedTangentsBuffer && PackedTangentsBuffer.IsBound()) {
      ShaderBindings.Add(PackedTangentsBuffer, pUserData->PackedTangentsBuffer);
    }
    if (pUserData->ColorBuffer && ColorBuffer.IsBound()) {
      ShaderBindings.Add(ColorBuffer, pUserData->ColorBuffer);
    }
    if (pUserData->TexCoordBuffer && TexCoordBuffer.IsBound()) {
      ShaderBindings.Add(TexCoordBuffer, pUserData->TexCoordBuffer);
    }
    if (pUserData->bHasVertexColors && bHasVertexColors.IsBound()) {
      ShaderBindings.Add(bHasVertexColors, pUserData->bHasVertexColors);
    }
    if (NumTexCoords.IsBound()) {
      ShaderBindings.Add(NumTexCoords, pUserData->NumTexCoords);
    }
    if (LineWidth.IsBound()) {
      ShaderBindings.Add(LineWidth, pUserData->LineWidth);
    }
    if (Pattern.IsBound()) {
      ShaderBindings.Add(Pattern, pUserData->Pattern);
    }
  }

private:
  LAYOUT_FIELD(FShaderResourceParameter, IndexBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, PositionBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, PackedTangentsBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, ColorBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, TexCoordBuffer);
  LAYOUT_FIELD(FShaderParameter, NumTexCoords);
  LAYOUT_FIELD(FShaderParameter, bHasVertexColors);
  LAYOUT_FIELD(FShaderParameter, LineWidth);
  LAYOUT_FIELD(FShaderParameter, Pattern);
};

FCesiumLineStyleVertexFactory::FCesiumLineStyleVertexFactory(
    ERHIFeatureLevel::Type InFeatureLevel,
    const FPositionVertexBuffer* PositionVertexBuffer)
    : FLocalVertexFactory(InFeatureLevel, "FCesiumLineStyleVertexFactory") {}

bool FCesiumLineStyleVertexFactory::ShouldCompilePermutation(
    const FVertexFactoryShaderPermutationParameters& Parameters) {
  if (!RHISupportsManualVertexFetch(Parameters.Platform)) {
    return false;
  }

  return Parameters.MaterialParameters.MaterialDomain == MD_Surface ||
         Parameters.MaterialParameters.bIsDefaultMaterial ||
         Parameters.MaterialParameters.bIsSpecialEngineMaterial;
}

void FCesiumLineStyleVertexFactory::ModifyCompilationEnvironment(
    const FVertexFactoryShaderPermutationParameters& Parameters,
    FShaderCompilerEnvironment& OutEnvironment) {
  FLocalVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
}

void FCesiumLineStyleVertexFactory::InitRHI(FRHICommandListBase& RHICmdList) {
  FVertexDeclarationElementList Elements;
  Elements.Add(AccessStreamComponent(
      FVertexStreamComponent(
          &GCesiumDummyVertexBuffer,
          0,
          sizeof(FVector3f),
          VET_Float3),
      0));
  InitDeclaration(Elements);
}

void FCesiumLineStyleVertexFactory::ReleaseRHI() {
  FVertexFactory::ReleaseRHI();
}

IMPLEMENT_TYPE_LAYOUT(FCesiumLineStyleVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(
    FCesiumLineStyleVertexFactory,
    SF_Vertex,
    FCesiumLineStyleVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(
    FCesiumLineStyleVertexFactory,
    "/Plugin/CesiumForUnreal/Private/CesiumLineStyleVertexFactory.ush",
    EVertexFactoryFlags::UsedWithMaterials |
        EVertexFactoryFlags::SupportsDynamicLighting |
        EVertexFactoryFlags::SupportsPositionOnly);
