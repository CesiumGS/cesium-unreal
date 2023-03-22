// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsVertexFactory.h"

#include "MeshBatch.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"

// The parameters to be passed as UserData to the shader.
struct FCesiumGltfPointsBatchElementUserData {
  // FRHIShaderResourceView* PointsBuffer;
  FVector AttenuationParameters;
  FVector4 ConstantColor;
};

// Binds the above shader parameters
class FCesiumGltfPointsVertexFactoryShaderParameters
    : public FVertexFactoryShaderParameters {
  DECLARE_INLINE_TYPE_LAYOUT(
      FCesiumGltfPointsVertexFactoryShaderParameters,
      NonVirtual);

public:
  void Bind(const FShaderParameterMap& ParameterMap) {
    // PointsBuffer.Bind(ParameterMap, TEXT("DataBuffer"), SPF_Mandatory);
    AttenuationParameters.Bind(
        ParameterMap,
        TEXT("AttenuationParameters"),
        SPF_Mandatory);
    ConstantColor.Bind(ParameterMap, TEXT("ConstantColor"), SPF_Mandatory);
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
    FCesiumGltfPointsBatchElementUserData* UserData =
        (FCesiumGltfPointsBatchElementUserData*)BatchElement.UserData;
    /* if (UserData->PointsBuffer && PointsBuffer.IsBound()) {
      ShaderBindings.Add(PointsBuffer, UserData->PointsBuffer);
    }*/

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
  // LAYOUT_FIELD(FShaderResourceParameter, PointsBuffer);
  LAYOUT_FIELD(FShaderParameter, AttenuationParameters);
  LAYOUT_FIELD(FShaderParameter, ConstantColor);
};

FCesiumGltfPointsVertexFactory::FCesiumGltfPointsVertexFactory(
    ERHIFeatureLevel::Type InFeatureLevel,
    const FStaticMeshVertexBuffers& InStaticMeshVertexBuffers)
    : FLocalVertexFactory(InFeatureLevel, "FCesiumGltfPointsVertexFactory"),
      StaticMeshVertexBuffers(InStaticMeshVertexBuffers) {}

bool FCesiumGltfPointsVertexFactory::ShouldCompilePermutation(
    const FVertexFactoryShaderPermutationParameters& Parameters) {
  return Parameters.MaterialParameters.MaterialDomain == MD_Surface ||
         Parameters.MaterialParameters.bIsDefaultMaterial;
}

void FCesiumGltfPointsVertexFactory::InitRHI() {
  check(HasValidFeatureLevel());

  //VertexBuffer.InitResource();

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

  InitDeclaration(Elements);
}

void FCesiumGltfPointsVertexFactory::ReleaseRHI() {
  FVertexFactory::ReleaseRHI();
  //VertexBuffer.ReleaseResource();
}

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(
    FCesiumGltfPointsVertexFactory,
    SF_Vertex,
    FCesiumGltfPointsVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(
    FCesiumGltfPointsVertexFactory,
    "/Plugin/CesiumForUnreal/Private/CesiumGltfPointsVertexFactory.ush",
    true, // bUsedWithMaterials
    true, // bSupportsStaticLighting
    true, // bSupportsDynamicLighting
    true, // bPrecisePrevWorldPos
    true  // bSupportsPositionOnly
);
