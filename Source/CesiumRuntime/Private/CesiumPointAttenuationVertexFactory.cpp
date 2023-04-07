// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPointAttenuationVertexFactory.h"

#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"

void FCesiumPointAttenuationIndexBuffer::InitRHI() {
  FRHIResourceCreateInfo CreateInfo;
  void* Buffer = nullptr;
  const uint32 NumIndices = NumPoints * 6;
  const uint32 Size = NumIndices * sizeof(uint32);

  IndexBufferRHI = RHICreateAndLockIndexBuffer(
      sizeof(uint32),
      Size,
      BUF_Static,
      CreateInfo,
      Buffer);

  uint32* Data = (uint32*)Buffer;
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

  RHIUnlockIndexBuffer(IndexBufferRHI);
  Buffer = nullptr;
}

class FCesiumPointAttenuationVertexFactoryShaderParameters
    : public FVertexFactoryShaderParameters {
  DECLARE_INLINE_TYPE_LAYOUT(
      FCesiumPointAttenuationVertexFactoryShaderParameters,
      NonVirtual);

public:
  void Bind(const FShaderParameterMap& ParameterMap) {
    PositionBuffer.Bind(ParameterMap, TEXT("PositionBuffer"), SPF_Mandatory);
    AttenuationParameters.Bind(
        ParameterMap,
        TEXT("AttenuationParameters"),
        SPF_Optional);
    ConstantColor.Bind(ParameterMap, TEXT("ConstantColor"), SPF_Optional);
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
  LAYOUT_FIELD(FShaderParameter, AttenuationParameters);
  LAYOUT_FIELD(FShaderParameter, ConstantColor);
};

FCesiumPointAttenuationVertexFactory::FCesiumPointAttenuationVertexFactory(
    ERHIFeatureLevel::Type InFeatureLevel,
    const FStaticMeshVertexBuffers& InStaticMeshVertexBuffers)
    : FLocalVertexFactory(InFeatureLevel, "FCesiumGltfPointsVertexFactory"),
      StaticMeshVertexBuffers(InStaticMeshVertexBuffers) {
  bNeedsDeclaration = false;
}

bool FCesiumPointAttenuationVertexFactory::ShouldCompilePermutation(
    const FVertexFactoryShaderPermutationParameters& Parameters) {
  return Parameters.MaterialParameters.MaterialDomain == MD_Surface ||
         Parameters.MaterialParameters.bIsDefaultMaterial;
}

void FCesiumPointAttenuationVertexFactory::InitRHI() {
  /* check(HasValidFeatureLevel());

 VertexBuffer.InitResource();

  FVertexDeclarationElementList Elements;

  Elements.Add(AccessStreamComponent(
      FVertexStreamComponent(
          &StaticMeshVertexBuffers.PositionVertexBuffer,
          0,
          sizeof(FVector),
          VET_Float3),
      0));
  Elements.Add(AccessStreamComponent(
      FVertexStreamComponent(
          &StaticMeshVertexBuffers.ColorVertexBuffer,
          0,
          sizeof(FVector4),
          VET_Float4),
      0));

  InitDeclaration(Elements);*/
}

void FCesiumPointAttenuationVertexFactory::ReleaseRHI() {
  FVertexFactory::ReleaseRHI();
  // VertexBuffer.ReleaseResource();
}

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(
    FCesiumPointAttenuationVertexFactory,
    SF_Vertex,
    FCesiumPointAttenuationVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(
    FCesiumPointAttenuationVertexFactory,
    "/Plugin/CesiumForUnreal/Private/CesiumPointAttenuationVertexFactory.ush",
    true,  // bUsedWithMaterials
    false, // bSupportsStaticLighting
    true,  // bSupportsDynamicLighting
    false, // bPrecisePrevWorldPos
    false  // bSupportsPositionOnly
);
