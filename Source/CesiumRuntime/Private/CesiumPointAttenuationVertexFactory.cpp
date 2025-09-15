// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPointAttenuationVertexFactory.h"

#include "CesiumVertexFactoryCommon.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialDomain.h"
#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "RenderCommandFence.h"
#include "Runtime/Launch/Resources/Version.h"

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

#if ENGINE_VERSION_5_5_OR_HIGHER
  OutEnvironment.SetDefine(TEXT("ENGINE_VERSION_5_5_OR_HIGHER"), TEXT("1"));
#endif
}

void FCesiumPointAttenuationVertexFactory::InitRHI(
    FRHICommandListBase& RHICmdList) {
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
