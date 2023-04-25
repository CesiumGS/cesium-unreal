// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPointAttenuationVertexFactory.h"

#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"

void FCesiumPointAttenuationIndexBuffer::InitRHI() {
  FRHIResourceCreateInfo CreateInfo(
      TEXT("FCesiumPointAttenuationIndexBufferCreateInfo"));
  const uint32 NumIndices = NumPoints * 6;
  const uint32 Size = NumIndices * sizeof(uint32);

  IndexBufferRHI = RHICreateBuffer(
      Size,
      BUF_Static | BUF_IndexBuffer,
      sizeof(uint32),
      ERHIAccess::VertexOrIndexBuffer,
      CreateInfo);

  uint32* Data = (uint32*)RHILockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);

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

  RHIUnlockBuffer(IndexBufferRHI);
}

class FCesiumPointAttenuationVertexFactoryShaderParameters
    : public FVertexFactoryShaderParameters {
  DECLARE_TYPE_LAYOUT(
      FCesiumPointAttenuationVertexFactoryShaderParameters,
      NonVirtual);

public:
  void Bind(const FShaderParameterMap& ParameterMap) {
    PositionBuffer.Bind(ParameterMap, TEXT("PositionBuffer"), SPF_Mandatory);
    PackedTangentsBuffer.Bind(ParameterMap, TEXT("PackedTangentsBuffer"));
    ColorBuffer.Bind(ParameterMap, TEXT("ColorBuffer"));
    bHasPointColors.Bind(ParameterMap, TEXT("bHasPointColors"));
    AttenuationParameters.Bind(ParameterMap, TEXT("AttenuationParameters"));
    ConstantColor.Bind(ParameterMap, TEXT("ConstantColor"));
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

    if (bHasPointColors.IsBound()) {
      ShaderBindings.Add(bHasPointColors, UserData->bHasPointColors);
    }

    if (AttenuationParameters.IsBound()) {
      ShaderBindings.Add(
          AttenuationParameters,
          UserData->AttenuationParameters);
    }

    if (ConstantColor.IsBound()) {
      ShaderBindings.Add(ConstantColor, UserData->ConstantColor);
    }
  }

private:
  LAYOUT_FIELD(FShaderResourceParameter, PositionBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, PackedTangentsBuffer);
  LAYOUT_FIELD(FShaderResourceParameter, ColorBuffer);
  LAYOUT_FIELD(FShaderParameter, bHasPointColors);
  LAYOUT_FIELD(FShaderParameter, AttenuationParameters);
  LAYOUT_FIELD(FShaderParameter, ConstantColor);
};

FCesiumPointAttenuationVertexFactory::FCesiumPointAttenuationVertexFactory(
    ERHIFeatureLevel::Type InFeatureLevel)
    : FLocalVertexFactory(InFeatureLevel, "FCesiumGltfPointsVertexFactory") {
  // Vertices are not declared in an explicit vertex stream, so set this false
  // to avoid errors.
  bNeedsDeclaration = false;
}

bool FCesiumPointAttenuationVertexFactory::ShouldCompilePermutation(
    const FVertexFactoryShaderPermutationParameters& Parameters) {
  return Parameters.MaterialParameters.MaterialDomain == MD_Surface ||
         Parameters.MaterialParameters.bIsDefaultMaterial;
}

void FCesiumPointAttenuationVertexFactory::InitRHI() {}

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
        EVertexFactoryFlags::SupportsDynamicLighting);

//| EVertexFactoryFlags::SupportsPositionOnly);
