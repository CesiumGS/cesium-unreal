// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfComponent.h"
#include "Async/Async.h"
#include "CesiumCommon.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumFeatureIdSet.h"
#include "CesiumGltfGaussianSplatComponent.h"
#include "CesiumGltfPointsComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfTextures.h"
#include "CesiumMaterialUserData.h"
#include "CesiumRasterOverlays.h"
#include "CesiumRuntime.h"
#include "CesiumTextureUtility.h"
#include "CesiumTransforms.h"
#include "Chaos/AABBTree.h"
#include "Chaos/CollisionConvexMesh.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "CreateGltfOptions.h"
#include "EncodedFeaturesMetadata.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "LoadGltfResult.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshTypes.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PixelFormat.h"
#include "Runtime/Launch/Resources/Version.h"
#include "StaticMeshOperations.h"
#include "StaticMeshResources.h"
#include "UObject/ConstructorHelpers.h"
#include "VecMath.h"
#include "mikktspace.h"

#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeometry/Transforms.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtInstanceFeatures.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/ExtensionKhrGaussianSplatting.h>
#include <CesiumGltf/ExtensionKhrMaterialsUnlit.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/TextureInfo.h>
#include <CesiumGltf/VertexAttributeSemantics.h>
#include <CesiumGltfContent/GltfUtilities.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Tracing.h>
#include <CesiumUtility/joinToString.h>
#include <cstddef>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <iostream>
#include <type_traits>

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

using namespace CesiumTextureUtility;
using namespace CreateGltfOptions;
using namespace LoadGltfResult;

// To debug which urls correspond to which gltf components you see in the view,
// - Set this define to 1
// - Click on a piece of terrain in the editor viewport to select it
// - Press delete to try to delete it
// Note that the console gives an error, but also tells you the url associated
// with it
#define DEBUG_GLTF_ASSET_NAMES 0

static uint32_t nextMaterialId = 0;

namespace {
class HalfConstructedReal : public UCesiumGltfComponent::HalfConstructed {
public:
  LoadedModelResult loadModelResult{};
};
} // namespace

template <class... T> struct IsAccessorView;

template <class T> struct IsAccessorView<T> : std::false_type {};

template <class T>
struct IsAccessorView<CesiumGltf::AccessorView<T>> : std::true_type {};

namespace {
uint32_t addAttributeAccessorToMap(
    const CesiumGltf::MeshPrimitive& primitive,
    const std::string& attributeName,
    std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap) {
  auto uvAccessorIt = primitive.attributes.find(attributeName);
  if (uvAccessorIt == primitive.attributes.end()) {
    // Texture not used, texture coordinates don't matter.
    return 0;
  }

  int32_t uvAccessorID = uvAccessorIt->second;
  size_t textureCoordinateIndex = gltfToUnrealTexCoordMap.size();

  // Use try_emplace to avoid overwriting an existing texture coordinate index
  // for the accessor.
  gltfToUnrealTexCoordMap.try_emplace(uvAccessorID, textureCoordinateIndex);
  return gltfToUnrealTexCoordMap[uvAccessorID];
}

template <class T>
uint32_t addTextureCoordinatesToMap(
    const CesiumGltf::MeshPrimitive& primitive,
    const std::optional<T>& maybeTexture,
    std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap) {
  if (!maybeTexture) {
    return 0;
  }

  return addAttributeAccessorToMap(
      primitive,
      "TEXCOORD_" + std::to_string(maybeTexture->texCoord),
      gltfToUnrealTexCoordMap);
}

void accumulateFeaturesMetadataAccessors(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const LoadedModelResult& modelResult,
    LoadedPrimitiveResult& primitiveResult) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::AccumulateAccessorsForFeaturesMetadata)

  std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap =
      primitiveResult.GltfToUnrealTexCoordMap;

  // Add any texture coordinates used for property textures present in the
  // primitive's metadata.
  for (const int64 propertyTextureIndex :
       primitiveResult.EncodedMetadata.propertyTextureIndices) {
    const EncodedFeaturesMetadata::EncodedPropertyTexture&
        encodedPropertyTexture =
            modelResult.EncodedMetadata.propertyTextures[propertyTextureIndex];

    for (const EncodedFeaturesMetadata::EncodedPropertyTextureProperty&
             encodedProperty : encodedPropertyTexture.properties) {

      FString fullPropertyName =
          EncodedFeaturesMetadata::getMaterialNameForPropertyTextureProperty(
              encodedPropertyTexture.name,
              encodedProperty.name);

      uint32 index = addAttributeAccessorToMap(
          primitive,
          "TEXCOORD_" +
              std::to_string(encodedProperty.textureCoordinateSetIndex),
          gltfToUnrealTexCoordMap);

      primitiveResult.FeaturesMetadataTexCoordParameters.Emplace(
          fullPropertyName +
              EncodedFeaturesMetadata::MaterialTexCoordIndexSuffix,
          index);
    }
  }

  // Add accessors used for feature IDs.
  for (const EncodedFeaturesMetadata::EncodedFeatureIdSet& encodedFeatureIDSet :
       primitiveResult.EncodedFeatures.featureIdSets) {
    FString SafeName =
        EncodedFeaturesMetadata::createHlslSafeName(encodedFeatureIDSet.name);

    if (encodedFeatureIDSet.attribute) {
      std::string attributeName =
          "_FEATURE_ID_" + std::to_string(*encodedFeatureIDSet.attribute);
      int32_t accessorIndex = primitive.attributes.at(attributeName);
      if (accessorIndex < 0)
        continue;

      primitiveResult.AccessorToFeatureIdIndexMap.emplace(
          accessorIndex,
          encodedFeatureIDSet.index);
      primitiveResult.FeaturesMetadataTexCoordParameters.Emplace(
          SafeName,
          addAttributeAccessorToMap(
              primitive,
              attributeName,
              gltfToUnrealTexCoordMap));
    } else if (encodedFeatureIDSet.texture) {
      int64 setIndex = encodedFeatureIDSet.texture->textureCoordinateSetIndex;
      std::string attributeName = "TEXCOORD_" + std::to_string(setIndex);

      primitiveResult.FeaturesMetadataTexCoordParameters.Emplace(
          SafeName + EncodedFeaturesMetadata::MaterialTexCoordIndexSuffix,
          addAttributeAccessorToMap(
              primitive,
              attributeName,
              gltfToUnrealTexCoordMap));
    } else {
      // Similar to feature ID attributes, we encode the unsigned integer
      // vertex ids as floats in the u-channel of a texture coordinate slot.
      // If it ever becomes possible to access the vertex ID through an
      // Unreal material node, this can be removed.
      uint32_t textureCoordinateIndex = gltfToUnrealTexCoordMap.size();

      // Use try_emplace to ensure a texture coordinate index is only assigned
      // the first time.
      gltfToUnrealTexCoordMap.try_emplace(-1, textureCoordinateIndex);
      primitiveResult.FeaturesMetadataTexCoordParameters.Emplace(
          SafeName,
          gltfToUnrealTexCoordMap[-1]);
    }
  }
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void accumulateFeaturesMetadataAccessors_DEPRECATED(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const LoadedModelResult& modelResult,
    LoadedPrimitiveResult& primitiveResult) {
  if (!primitiveResult.EncodedMetadata_DEPRECATED)
    return;

  CesiumEncodedMetadataUtility::EncodedMetadataPrimitive&
      encodedPrimitiveMetadata = *primitiveResult.EncodedMetadata_DEPRECATED;
  std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap =
      primitiveResult.GltfToUnrealTexCoordMap;

  TRACE_CPUPROFILER_EVENT_SCOPE(
      Cesium::AccumulateAccessorsForFeaturesMetadata_DEPRECATED)

  for (const CesiumEncodedMetadataUtility::EncodedFeatureIdTexture&
           encodedFeatureIdTexture :
       encodedPrimitiveMetadata.encodedFeatureIdTextures) {
    primitiveResult.FeaturesMetadataTexCoordParameters.Emplace(
        encodedFeatureIdTexture.baseName + "UV",
        addAttributeAccessorToMap(
            primitive,
            "TEXCOORD_" +
                std::to_string(
                    encodedFeatureIdTexture.textureCoordinateAttributeId),
            gltfToUnrealTexCoordMap));
  }

  if (modelResult.EncodedMetadata_DEPRECATED) {
    const CesiumEncodedMetadataUtility::EncodedMetadata& encodedMetadata =
        *modelResult.EncodedMetadata_DEPRECATED;

    for (const FString& featureTextureName :
         encodedPrimitiveMetadata.featureTextureNames) {
      const CesiumEncodedMetadataUtility::EncodedFeatureTexture*
          pEncodedFeatureTexture =
              encodedMetadata.encodedFeatureTextures.Find(featureTextureName);
      if (pEncodedFeatureTexture) {
        for (const CesiumEncodedMetadataUtility::EncodedFeatureTextureProperty&
                 encodedProperty : pEncodedFeatureTexture->properties) {
          primitiveResult.FeaturesMetadataTexCoordParameters.Emplace(
              encodedProperty.baseName + "UV",
              addAttributeAccessorToMap(
                  primitive,
                  "TEXCOORD_" +
                      std::to_string(
                          encodedProperty.textureCoordinateAttributeId),
                  gltfToUnrealTexCoordMap));
        }
      }
    }
  }

  const CesiumGltf::ExtensionExtMeshFeatures* pFeatures =
      primitive.getExtension<CesiumGltf::ExtensionExtMeshFeatures>();

  if (pFeatures) {
    TArray<FCesiumFeatureIdAttribute> featureIdAttributes =
        UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
            primitiveResult.Metadata_DEPRECATED);

    for (const CesiumEncodedMetadataUtility::EncodedFeatureIdAttribute&
             encodedFeatureIdAttribute :
         encodedPrimitiveMetadata.encodedFeatureIdAttributes) {
      const FCesiumFeatureIdAttribute& featureIdAttribute =
          featureIdAttributes[encodedFeatureIdAttribute.index];

      int32_t attribute = featureIdAttribute.getAttributeIndex();
      std::string attributeName = "_FEATURE_ID_" + std::to_string(attribute);
      int32_t accessor = primitive.attributes.at(attributeName);
      if (accessor < 0)
        continue;

      primitiveResult.AccessorToFeatureIdIndexMap.emplace(
          accessor,
          encodedFeatureIdAttribute.index);
      primitiveResult.FeaturesMetadataTexCoordParameters.Emplace(
          encodedFeatureIdAttribute.name,
          addAttributeAccessorToMap(
              primitive,
              attributeName,
              gltfToUnrealTexCoordMap));
    }
  }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void accumulateMaterialAndOverlayTextureCoordinates(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::Material& material,
    const CesiumGltf::MaterialPBRMetallicRoughness& pbrMetallicRoughness,
    LoadedPrimitiveResult& primitiveResult) {
  std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap =
      primitiveResult.GltfToUnrealTexCoordMap;

  primitiveResult
      .textureCoordinateParameters["baseColorTextureCoordinateIndex"] =
      addTextureCoordinatesToMap(
          primitive,
          pbrMetallicRoughness.baseColorTexture,
          gltfToUnrealTexCoordMap);
  primitiveResult
      .textureCoordinateParameters["metallicRoughnessTextureCoordinateIndex"] =
      addTextureCoordinatesToMap(
          primitive,
          pbrMetallicRoughness.metallicRoughnessTexture,
          gltfToUnrealTexCoordMap);
  primitiveResult.textureCoordinateParameters["normalTextureCoordinateIndex"] =
      addTextureCoordinatesToMap(
          primitive,
          material.normalTexture,
          gltfToUnrealTexCoordMap);
  primitiveResult
      .textureCoordinateParameters["occlusionTextureCoordinateIndex"] =
      addTextureCoordinatesToMap(
          primitive,
          material.occlusionTexture,
          gltfToUnrealTexCoordMap);
  primitiveResult
      .textureCoordinateParameters["emissiveTextureCoordinateIndex"] =
      addTextureCoordinatesToMap(
          primitive,
          material.emissiveTexture,
          gltfToUnrealTexCoordMap);

  for (size_t i = 0;
       i < primitiveResult.overlayTextureCoordinateIDToUVIndex.size();
       ++i) {
    primitiveResult.overlayTextureCoordinateIDToUVIndex[i] =
        addAttributeAccessorToMap(
            primitive,
            "_CESIUMOVERLAY_" + std::to_string(i),
            gltfToUnrealTexCoordMap);
  }
}

void copyFeatureIds(
    const FCesiumPrimitiveFeatures& primitiveFeatures,
    int32_t primitiveFeaturesIndex,
    uint32_t textureCoordinateIndex,
    FStaticMeshVertexBuffer& vertices,
    const TArray<uint32>& indices,
    bool duplicateVertices) {
  const TArray<FCesiumFeatureIdSet>& featureIdSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
          primitiveFeatures);
  if (primitiveFeaturesIndex < 0 ||
      primitiveFeaturesIndex >= featureIdSets.Num())
    return;

  const FCesiumFeatureIdSet& featureIdSet =
      featureIdSets[primitiveFeaturesIndex];
  const FCesiumFeatureIdAttribute& featureIdAttribute =
      UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
          featureIdSet);

  // We encode unsigned integer feature ids as floats in the u-channel of
  // a texture coordinate slot.
  if (duplicateVertices) {
    for (int64_t i = 0; i < indices.Num(); ++i) {
      uint32 vertexIndex = indices[i];
      float featureId = UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
          featureIdAttribute,
          vertexIndex);
      vertices.SetVertexUV(
          i,
          textureCoordinateIndex,
          FVector2f(featureId, 0.0f));
    }
  } else {
    for (int64_t i = 0; i < vertices.GetNumVertices(); ++i) {
      float featureId = UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
          featureIdAttribute,
          i);
      vertices.SetVertexUV(
          i,
          textureCoordinateIndex,
          FVector2f(featureId, 0.0f));
    }
  }
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void copyFeatureIds_DEPRECATED(
    const FCesiumMetadataPrimitive& metadataPrimitive,
    int32_t attributeIndex,
    uint32_t textureCoordinateIndex,
    FStaticMeshVertexBuffer& vertices,
    const TArray<uint32>& indices,
    bool duplicateVertices) {
  TArray<FCesiumFeatureIdAttribute> featureIdAttributes =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
          metadataPrimitive);
  if (attributeIndex < 0 || attributeIndex >= featureIdAttributes.Num())
    return;

  const FCesiumFeatureIdAttribute& featureIdAttribute =
      featureIdAttributes[attributeIndex];

  // We encode unsigned integer feature ids as floats in the u-channel of
  // a texture coordinate slot.
  if (duplicateVertices) {
    for (int64_t i = 0; i < indices.Num(); ++i) {
      uint32 vertexIndex = indices[i];
      float featureId = UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
          featureIdAttribute,
          vertexIndex);
      vertices.SetVertexUV(
          i,
          textureCoordinateIndex,
          FVector2f(glm::max(featureId, 0.0f), 0.0f));
    }
  } else {
    for (int64_t i = 0; i < vertices.GetNumVertices(); ++i) {
      float featureId = UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
          featureIdAttribute,
          i);
      vertices.SetVertexUV(
          i,
          textureCoordinateIndex,
          FVector2f(glm::max(featureId, 0.0f), 0.0f));
    }
  }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

inline FVector2f getUVOrDefault(
    const CesiumGltf::AccessorView<FVector2f>& uvAccessor,
    int32 index) {
  return index >= 0 && index < uvAccessor.size() ? uvAccessor[index]
                                                 : FVector2f(0.0f, 0.0f);
}

void copyTextureCoordinates(
    const CesiumGltf::Model& model,
    int32_t accessorIndex,
    uint32_t textureCoordinateIndex,
    FStaticMeshVertexBuffer& vertices,
    const TArray<uint32>& indices,
    bool duplicateVertices) {
  CesiumGltf::AccessorView<FVector2f> uvAccessor(model, accessorIndex);
  if (uvAccessor.status() != CesiumGltf::AccessorViewStatus::Valid) {
    return;
  }

  if (duplicateVertices) {
    for (int i = 0; i < indices.Num(); ++i) {
      vertices.SetVertexUV(
          i,
          textureCoordinateIndex,
          getUVOrDefault(uvAccessor, indices[i]));
    }
  } else {
    for (uint32 i = 0; i < vertices.GetNumVertices(); ++i) {
      vertices.SetVertexUV(
          i,
          textureCoordinateIndex,
          getUVOrDefault(uvAccessor, i));
    }
  }
}

void populateUnrealTexCoords(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CreateModelOptions& modelOptions,
    FStaticMeshVertexBuffer& vertices,
    const TArray<uint32>& indices,
    bool duplicateVertices,
    LoadedPrimitiveResult& result) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::UpdateTextureCoordinates)

  std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap =
      result.GltfToUnrealTexCoordMap;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  if (modelOptions.pFeaturesMetadataDescription) {
    for (const auto indexPair : result.AccessorToFeatureIdIndexMap) {
      copyFeatureIds(
          result.Features,
          indexPair.second,
          gltfToUnrealTexCoordMap[indexPair.first],
          vertices,
          indices,
          duplicateVertices);
    }
  } else if (modelOptions.pEncodedMetadataDescription_DEPRECATED) {
    for (const auto indexPair : result.AccessorToFeatureIdIndexMap) {
      copyFeatureIds_DEPRECATED(
          result.Metadata_DEPRECATED,
          indexPair.second,
          gltfToUnrealTexCoordMap[indexPair.first],
          vertices,
          indices,
          duplicateVertices);
    }
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  for (const auto indexPair : gltfToUnrealTexCoordMap) {
    if (result.AccessorToFeatureIdIndexMap.contains(indexPair.first)) {
      continue;
    }
    copyTextureCoordinates(
        model,
        indexPair.first,
        indexPair.second,
        vertices,
        indices,
        duplicateVertices);
  }

  if (gltfToUnrealTexCoordMap.contains(-1)) {
    uint32_t textureCoordinateIndex = gltfToUnrealTexCoordMap[-1];
    if (duplicateVertices) {
      for (int32 i = 0; i < indices.Num(); ++i) {
        uint32 vertexIndex = indices[i];
        vertices.SetVertexUV(
            uint32(i),
            textureCoordinateIndex,
            FVector2f(float(vertexIndex), 0.0f));
      }
    } else {
      for (uint32 i = 0; i < vertices.GetNumVertices(); ++i) {
        vertices.SetVertexUV(
            i,
            textureCoordinateIndex,
            FVector2f(float(i), 0.0f));
      }
    }
  }
}
} // namespace

static int mikkGetNumFaces(const SMikkTSpaceContext* Context) {
  const FStaticMeshVertexBuffers& vertices =
      *reinterpret_cast<FStaticMeshVertexBuffers*>(Context->m_pUserData);
  return int32(vertices.PositionVertexBuffer.GetNumVertices() / 3);
}

static int
mikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx) {
  const FStaticMeshVertexBuffers& vertices =
      *reinterpret_cast<FStaticMeshVertexBuffers*>(Context->m_pUserData);
  return FaceIdx < int32(vertices.PositionVertexBuffer.GetNumVertices() / 3)
             ? 3
             : 0;
}

static void mikkGetPosition(
    const SMikkTSpaceContext* Context,
    float Position[3],
    const int FaceIdx,
    const int VertIdx) {
  const FStaticMeshVertexBuffers& vertices =
      *reinterpret_cast<FStaticMeshVertexBuffers*>(Context->m_pUserData);

  const FPositionVertexBuffer& positionBuffer = vertices.PositionVertexBuffer;
  uint32 vertexIndex = uint32(FaceIdx * 3 + VertIdx);

  const FVector3f& position = positionBuffer.VertexPosition(vertexIndex);
  Position[0] = position.X;
  Position[1] = -position.Y;
  Position[2] = position.Z;
}

static void mikkGetNormal(
    const SMikkTSpaceContext* Context,
    float Normal[3],
    const int FaceIdx,
    const int VertIdx) {
  const FStaticMeshVertexBuffers& vertices =
      *reinterpret_cast<FStaticMeshVertexBuffers*>(Context->m_pUserData);

  const FStaticMeshVertexBuffer& vertexBuffer = vertices.StaticMeshVertexBuffer;
  uint32 vertexIndex = uint32(FaceIdx * 3 + VertIdx);

  FVector3f normal = vertexBuffer.VertexTangentZ(vertexIndex);
  Normal[0] = normal.X;
  Normal[1] = -normal.Y;
  Normal[2] = normal.Z;
}

static void mikkGetTexCoord(
    const SMikkTSpaceContext* Context,
    float UV[2],
    const int FaceIdx,
    const int VertIdx) {
  const FStaticMeshVertexBuffers& vertices =
      *reinterpret_cast<FStaticMeshVertexBuffers*>(Context->m_pUserData);

  const FStaticMeshVertexBuffer& vertexBuffer = vertices.StaticMeshVertexBuffer;
  uint32 vertexIndex = uint32(FaceIdx * 3 + VertIdx);

  FVector2f uv = vertexBuffer.GetVertexUV(vertexIndex, 0);
  UV[0] = uv.X;
  UV[1] = uv.Y;
}

static void mikkSetTSpaceBasic(
    const SMikkTSpaceContext* Context,
    const float Tangent[3],
    const float BitangentSign,
    const int FaceIdx,
    const int VertIdx) {
  FStaticMeshVertexBuffers& vertices =
      *reinterpret_cast<FStaticMeshVertexBuffers*>(Context->m_pUserData);

  FStaticMeshVertexBuffer& vertexBuffer = vertices.StaticMeshVertexBuffer;
  uint32 vertexIndex = uint32(FaceIdx * 3 + VertIdx);

  FVector3f TangentZ = vertexBuffer.VertexTangentZ(vertexIndex);
  TangentZ.Y = -TangentZ.Y;

  FVector3f TangentX = FVector3f(Tangent[0], Tangent[1], Tangent[2]);
  FVector3f TangentY =
      BitangentSign * FVector3f::CrossProduct(TangentZ, TangentX);

  TangentX.Y = -TangentX.Y;
  TangentY.Y = -TangentY.Y;
  TangentZ.Y = -TangentZ.Y;

  vertexBuffer.SetVertexTangents(vertexIndex, TangentX, TangentY, TangentZ);
}

static void computeTangentSpace(FStaticMeshVertexBuffers& vertices) {
  SMikkTSpaceInterface MikkTInterface{};
  MikkTInterface.m_getNormal = mikkGetNormal;
  MikkTInterface.m_getNumFaces = mikkGetNumFaces;
  MikkTInterface.m_getNumVerticesOfFace = mikkGetNumVertsOfFace;
  MikkTInterface.m_getPosition = mikkGetPosition;
  MikkTInterface.m_getTexCoord = mikkGetTexCoord;
  MikkTInterface.m_setTSpaceBasic = mikkSetTSpaceBasic;
  MikkTInterface.m_setTSpace = nullptr;

  SMikkTSpaceContext MikkTContext{};
  MikkTContext.m_pInterface = &MikkTInterface;
  MikkTContext.m_pUserData = (void*)(&vertices);
  // MikkTContext.m_bIgnoreDegenerates = false;
  genTangSpaceDefault(&MikkTContext);
}

static void setUnlitNormals(
    FStaticMeshVertexBuffers& vertices,
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const glm::dmat4& vertexToEllipsoidFixed) {
  glm::dmat4 ellipsoidFixedToVertex =
      glm::affineInverse(vertexToEllipsoidFixed);

  const FPositionVertexBuffer& positionBuffer = vertices.PositionVertexBuffer;
  FStaticMeshVertexBuffer& normalBuffer = vertices.StaticMeshVertexBuffer;

  int32 numVertices = positionBuffer.GetNumVertices();
  for (int i = 0; i < numVertices; i++) {
    glm::dvec3 positionFixed = glm::dvec3(
        vertexToEllipsoidFixed *
        glm::dvec4(
            VecMath::createVector3D(FVector(positionBuffer.VertexPosition(i))),
            1.0));
    glm::dvec3 normal = ellipsoid.geodeticSurfaceNormal(positionFixed);

    normalBuffer.SetVertexTangents(
        i,
        FVector3f(0.0f),
        FVector3f(0.0),
        FVector3f(VecMath::createVector(
            glm::normalize(ellipsoidFixedToVertex * glm::dvec4(normal, 0.0)))));
  }
}

static void computeFlatNormals(FStaticMeshVertexBuffers& vertices) {
  const FPositionVertexBuffer& positionBuffer = vertices.PositionVertexBuffer;
  FStaticMeshVertexBuffer& normalBuffer = vertices.StaticMeshVertexBuffer;

  int32 numVertices = positionBuffer.GetNumVertices();
  for (int i = 0; i < numVertices; i += 3) {
    const FVector3f& p0 = positionBuffer.VertexPosition(i);
    const FVector3f& p1 = positionBuffer.VertexPosition(i + 1);
    const FVector3f& p2 = positionBuffer.VertexPosition(i + 2);

    // The Y axis has previously been inverted, so undo that before
    // computing the normal direction. Then invert the Y coordinate of the
    // normal, too.

    FVector3f v01 = p1 - p0;
    v01.Y = -v01.Y;
    FVector3f v02 = p2 - p0;
    v02.Y = -v02.Y;
    FVector3f normal = FVector3f::CrossProduct(v01, v02);
    normal.Y = -normal.Y;

    FVector3f safeNormal = normal.GetSafeNormal();

    for (int vertexOffset = 0; vertexOffset < 3; vertexOffset++) {
      normalBuffer.SetVertexTangents(
          i + vertexOffset,
          FVector3f(0.0f),
          FVector3f(0.0f),
          safeNormal);
    }
  }
}

template <typename TIndex>
static Chaos::FTriangleMeshImplicitObjectPtr BuildChaosTriangleMeshes(
    const FPositionVertexBuffer& vertexBuffer,
    const TArray<uint32>& indices);

static const CesiumGltf::Material defaultMaterial;
static const CesiumGltf::MaterialPBRMetallicRoughness
    defaultPbrMetallicRoughness;

struct ColorVisitor {
  bool duplicateVertices;
  FColorVertexBuffer& colorBuffer;
  const TArray<uint32>& indices;

  bool operator()(CesiumGltf::AccessorView<nullptr_t>&& invalidView) {
    return false;
  }

  template <typename TColorView> bool operator()(TColorView&& colorView) {
    if (colorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      return false;
    }

    bool success = true;
    if (duplicateVertices) {
      for (int i = 0; success && i < this->indices.Num(); ++i) {
        uint32 vertexIndex = this->indices[i];
        if (vertexIndex >= colorView.size()) {
          success = false;
        } else {
          success = ColorVisitor::convertColor(
              colorView[vertexIndex],
              this->colorBuffer.VertexColor(i));
        }
      }
    } else {
      for (uint32 i = 0; success && i < this->colorBuffer.GetNumVertices();
           ++i) {
        if (i >= colorView.size()) {
          success = false;
        } else {
          success = ColorVisitor::convertColor(
              colorView[i],
              this->colorBuffer.VertexColor(i));
        }
      }
    }

    return success;
  }

  template <typename TElement>
  static bool convertColor(
      const CesiumGltf::AccessorTypes::VEC3<TElement>& color,
      FColor& out) {
    out.A = 255;
    return convertElement(color.value[0], out.R) &&
           convertElement(color.value[1], out.G) &&
           convertElement(color.value[2], out.B);
  }

  template <typename TElement>
  static bool convertColor(
      const CesiumGltf::AccessorTypes::VEC4<TElement>& color,
      FColor& out) {
    return convertElement(color.value[0], out.R) &&
           convertElement(color.value[1], out.G) &&
           convertElement(color.value[2], out.B) &&
           convertElement(color.value[3], out.A);
  }

  static bool convertElement(float value, uint8_t& out) {
    out = uint8_t(value * 255.0f);
    return true;
  }

  static bool convertElement(uint8_t value, uint8_t& out) {
    out = value;
    return true;
  }

  static bool convertElement(uint16_t value, uint8_t& out) {
    out = uint8_t(value / 256);
    return true;
  }

  template <typename T> static bool convertColor(const T& color, FColor& out) {
    return false;
  }

  template <typename T>
  static bool convertElement(const T& color, uint8_t& out) {
    return false;
  }
};

template <class T>
static TUniquePtr<CesiumTextureUtility::LoadedTextureResult> loadTexture(
    CesiumGltf::Model& model,
    const std::optional<T>& gltfTextureInfo,
    bool sRGB) {
  if (!gltfTextureInfo || gltfTextureInfo.value().index < 0 ||
      gltfTextureInfo.value().index >= model.textures.size()) {
    if (gltfTextureInfo && gltfTextureInfo.value().index >= 0) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Texture index must be less than %d, but is %d"),
          model.textures.size(),
          gltfTextureInfo.value().index);
    }
    return nullptr;
  }

  int32_t textureIndex = gltfTextureInfo.value().index;
  CesiumGltf::Texture& texture = model.textures[textureIndex];
  return loadTextureFromModelAnyThreadPart(model, texture, sRGB);
}

static void applyWaterMask(
    CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    LoadedPrimitiveResult& primitiveResult) {
  // Initialize water mask if needed.
  auto onlyWaterIt = primitive.extras.find("OnlyWater");
  auto onlyLandIt = primitive.extras.find("OnlyLand");
  if (onlyWaterIt != primitive.extras.end() && onlyWaterIt->second.isBool() &&
      onlyLandIt != primitive.extras.end() && onlyLandIt->second.isBool()) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ApplyWaterMask)
    bool onlyWater = onlyWaterIt->second.getBoolOrDefault(false);
    bool onlyLand = onlyLandIt->second.getBoolOrDefault(true);
    primitiveResult.onlyWater = onlyWater;
    primitiveResult.onlyLand = onlyLand;
    if (!onlyWater && !onlyLand) {
      // We have to use the water mask
      auto waterMaskTextureIdIt = primitive.extras.find("WaterMaskTex");
      if (waterMaskTextureIdIt != primitive.extras.end() &&
          waterMaskTextureIdIt->second.isInt64()) {
        int32_t waterMaskTextureId = static_cast<int32_t>(
            waterMaskTextureIdIt->second.getInt64OrDefault(-1));
        CesiumGltf::TextureInfo waterMaskInfo;
        waterMaskInfo.index = waterMaskTextureId;
        if (waterMaskTextureId >= 0 &&
            waterMaskTextureId < model.textures.size()) {
          primitiveResult.waterMaskTexture =
              loadTexture(model, std::make_optional(waterMaskInfo), false);
        }
      }
    }
  } else {
    primitiveResult.onlyWater = false;
    primitiveResult.onlyLand = true;
  }

  auto waterMaskTranslationXIt = primitive.extras.find("WaterMaskTranslationX");
  auto waterMaskTranslationYIt = primitive.extras.find("WaterMaskTranslationY");
  auto waterMaskScaleIt = primitive.extras.find("WaterMaskScale");

  if (waterMaskTranslationXIt != primitive.extras.end() &&
      waterMaskTranslationXIt->second.isDouble() &&
      waterMaskTranslationYIt != primitive.extras.end() &&
      waterMaskTranslationYIt->second.isDouble() &&
      waterMaskScaleIt != primitive.extras.end() &&
      waterMaskScaleIt->second.isDouble()) {
    primitiveResult.waterMaskTranslationX =
        waterMaskTranslationXIt->second.getDoubleOrDefault(0.0);
    primitiveResult.waterMaskTranslationY =
        waterMaskTranslationYIt->second.getDoubleOrDefault(0.0);
    primitiveResult.waterMaskScale =
        waterMaskScaleIt->second.getDoubleOrDefault(1.0);
  }
}

#pragma region Features Metadata helper functions(load thread)

static bool textureUsesSpecifiedImage(
    const CesiumGltf::Model& model,
    int32_t textureIndex,
    int32_t imageIndex) {
  if (textureIndex < 0 || textureIndex >= model.textures.size()) {
    return false;
  }

  const CesiumGltf::Texture& texture = model.textures[textureIndex];
  return texture.source == imageIndex;
}

static bool hasMaterialTextureConflicts(
    const CesiumGltf::Model& model,
    const CesiumGltf::Material& material,
    int32_t imageIndex) {
  if (material.pbrMetallicRoughness) {
    const std::optional<CesiumGltf::TextureInfo>& maybeBaseColorTexture =
        material.pbrMetallicRoughness->baseColorTexture;
    if (maybeBaseColorTexture && textureUsesSpecifiedImage(
                                     model,
                                     maybeBaseColorTexture->index,
                                     imageIndex)) {
      return true;
    }

    const std::optional<CesiumGltf::TextureInfo>&
        maybeMetallicRoughnessTexture =
            material.pbrMetallicRoughness->metallicRoughnessTexture;
    if (maybeMetallicRoughnessTexture &&
        textureUsesSpecifiedImage(
            model,
            maybeMetallicRoughnessTexture->index,
            imageIndex)) {
      return true;
    }
  }

  if (material.normalTexture && textureUsesSpecifiedImage(
                                    model,
                                    material.normalTexture->index,
                                    imageIndex)) {
    return true;
  }

  if (material.emissiveTexture && textureUsesSpecifiedImage(
                                      model,
                                      material.emissiveTexture->index,
                                      imageIndex)) {
    return true;
  }

  if (material.occlusionTexture && textureUsesSpecifiedImage(
                                       model,
                                       material.occlusionTexture->index,
                                       imageIndex)) {
    return true;
  }

  return false;
}

/**
 * Creates texture coordinate accessors for the feature ID sets and metadata
 * in the primitive. This enables feature ID texture / property texture
 * picking without requiring UVs in the physics bodies.
 */
static void createTexCoordAccessorsForFeaturesMetadata(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const FCesiumPrimitiveFeatures& primitiveFeatures,
    const FCesiumPrimitiveMetadata& primitiveMetadata,
    const FCesiumModelMetadata& modelMetadata,
    std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>&
        texCoordAccessorsMap) {
  auto featureIdTextures =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
          primitiveFeatures,
          ECesiumFeatureIdSetType::Texture);

  for (const FCesiumFeatureIdSet& featureIdSet : featureIdTextures) {
    FCesiumFeatureIdTexture featureIdTexture =
        UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
            featureIdSet);

    int64 gltfTexCoordSetIndex = UCesiumFeatureIdTextureBlueprintLibrary::
        GetGltfTextureCoordinateSetIndex(featureIdTexture);
    if (gltfTexCoordSetIndex < 0 ||
        texCoordAccessorsMap.find(gltfTexCoordSetIndex) !=
            texCoordAccessorsMap.end()) {
      // Skip if the index is invalid or if it has already been accounted for.
      continue;
    }
    texCoordAccessorsMap.emplace(
        gltfTexCoordSetIndex,
        CesiumGltf::getTexCoordAccessorView(
            model,
            primitive,
            gltfTexCoordSetIndex));
  }

  auto propertyTextureIndices =
      UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyTextureIndices(
          primitiveMetadata);
  auto propertyTextures =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTexturesAtIndices(
          modelMetadata,
          propertyTextureIndices);

  for (const FCesiumPropertyTexture& propertyTexture : propertyTextures) {
    auto properties =
        UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture);

    for (const auto& propertyIt : properties) {
      int64 gltfTexCoordSetIndex =
          UCesiumPropertyTexturePropertyBlueprintLibrary::
              GetGltfTextureCoordinateSetIndex(propertyIt.Value);
      if (gltfTexCoordSetIndex < 0 ||
          texCoordAccessorsMap.find(gltfTexCoordSetIndex) !=
              texCoordAccessorsMap.end()) {
        // Skip if the index is invalid or if it has already been accounted
        // for.
        continue;
      }
      texCoordAccessorsMap.emplace(
          gltfTexCoordSetIndex,
          CesiumGltf::getTexCoordAccessorView(
              model,
              primitive,
              gltfTexCoordSetIndex));
    }
  }
}

static void loadPrimitiveFeaturesMetadata(
    LoadedPrimitiveResult& primitiveResult,
    const CreatePrimitiveOptions& options,
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive) {

  CesiumGltf::ExtensionExtMeshFeatures* pFeatures =
      primitive.getExtension<CesiumGltf::ExtensionExtMeshFeatures>();

  if (pFeatures) {
    int32_t materialIndex = primitive.material;
    if (materialIndex >= 0 && materialIndex < model.materials.size()) {
      const CesiumGltf::Material& material =
          model.materials[primitive.material];

      for (CesiumGltf::FeatureId& featureId : pFeatures->featureIds) {
        if (!featureId.texture) {
          continue;
        }

        if (featureId.texture->extras.find("makeImageCopy") !=
            featureId.texture->extras.end()) {
          continue;
        }

        int32_t textureIndex = featureId.texture->index;
        if (textureIndex < 0 || textureIndex >= model.textures.size()) {
          continue;
        }

        const CesiumGltf::Texture& texture = model.textures[textureIndex];
        if (texture.source < 0 || texture.source >= model.images.size()) {
          continue;
        }

        int32_t imageIndex = model.textures[textureIndex].source;
        if (hasMaterialTextureConflicts(model, material, imageIndex)) {
          // Add a flag in the extras to indicate a copy should be made.
          // This is checked for in FCesiumFeatureIdTexture.
          featureId.texture->extras.insert({"makeImageCopy", true});
        }
      }
    }
  }

  const CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata* pMetadata =
      primitive.getExtension<
          CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata>();

  const CreateGltfOptions::CreateModelOptions* pModelOptions =
      options.pMeshOptions->pNodeOptions->pModelOptions;
  const LoadGltfResult::LoadedModelResult* pModelResult =
      options.pMeshOptions->pNodeOptions->pHalfConstructedModelResult;

  primitiveResult.Features =
      pFeatures ? FCesiumPrimitiveFeatures(model, primitive, *pFeatures)
                : FCesiumPrimitiveFeatures();
  primitiveResult.Metadata =
      pMetadata ? FCesiumPrimitiveMetadata(model, primitive, *pMetadata)
                : FCesiumPrimitiveMetadata();

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  primitiveResult.Metadata_DEPRECATED = FCesiumMetadataPrimitive{
      primitiveResult.Features,
      primitiveResult.Metadata,
      pModelResult->Metadata};
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  createTexCoordAccessorsForFeaturesMetadata(
      model,
      primitive,
      primitiveResult.Features,
      primitiveResult.Metadata,
      pModelResult->Metadata,
      primitiveResult.TexCoordAccessorMap);

  const FCesiumFeaturesMetadataDescription* pFeaturesMetadataDescription =
      pModelOptions->pFeaturesMetadataDescription;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  // Check for deprecated metadata description
  const FMetadataDescription* pMetadataDescription_DEPRECATED =
      pModelOptions->pEncodedMetadataDescription_DEPRECATED;

  if (pFeaturesMetadataDescription) {
    primitiveResult.EncodedFeatures =
        EncodedFeaturesMetadata::encodePrimitiveFeaturesAnyThreadPart(
            pFeaturesMetadataDescription->PrimitiveFeatures,
            primitiveResult.Features);

    primitiveResult.EncodedMetadata =
        EncodedFeaturesMetadata::encodePrimitiveMetadataAnyThreadPart(
            pFeaturesMetadataDescription->PrimitiveMetadata,
            primitiveResult.Metadata,
            pModelResult->Metadata);
  } else if (pMetadataDescription_DEPRECATED) {
    primitiveResult.EncodedMetadata_DEPRECATED =
        CesiumEncodedMetadataUtility::encodeMetadataPrimitiveAnyThreadPart(
            *pMetadataDescription_DEPRECATED,
            primitiveResult.Metadata_DEPRECATED);
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
}
#pragma endregion

namespace {

/**
 * @brief Constrain the length of the given string.
 *
 * If the string is shorter than the maximum length, it is returned.
 * If it is not longer than 3 characters, the first maxLength
 * characters will be returned.
 * Otherwise, the result will be of the form `prefix + "..." + suffix`,
 * with the prefix and suffix chosen so that the length of the result
 * is maxLength
 *
 * @param s The input string
 * @param maxLength The maximum length.
 * @return The constrained string
 */
std::string constrainLength(const std::string& s, const size_t maxLength) {
  if (s.length() <= maxLength) {
    return s;
  }
  if (maxLength <= 3) {
    return s.substr(0, maxLength);
  }
  const std::string ellipsis("...");
  const size_t prefixLength = ((maxLength - ellipsis.length()) + 1) / 2;
  const size_t suffixLength = (maxLength - ellipsis.length()) / 2;
  const std::string prefix = s.substr(0, prefixLength);
  const std::string suffix = s.substr(s.length() - suffixLength, suffixLength);
  return prefix + ellipsis + suffix;
}

/**
 * @brief Create an FName from the given strings.
 *
 * This will combine the prefix and the suffix and create an FName.
 * If the string would be longer than the given length, then
 * the prefix will be shortened (in an unspecified way), to
 * constrain the result to a length of maxLength.
 *
 * The default maximum length is 256, because Unreal may in turn
 * add a prefix like the `/Internal/Path/Name` to this name.
 *
 * @param prefix The prefix input string
 * @param suffix The suffix input string
 * @param maxLength The maximum length
 * @return The FName
 */
FName createSafeName(
    const std::string& prefix,
    const std::string& suffix,
    const size_t maxLength = 256) {
  std::string constrainedPrefix =
      constrainLength(prefix, maxLength - suffix.length());
  std::string combined = constrainedPrefix + suffix;
  return FName(combined.c_str());
}

// This matrix converts from right-handed Z-up to Unreal
// left-handed Z-up by flipping the Y axis. It effectively undoes the Y-axis
// flipping that we did when creating the mesh in the first place. This is
// necessary to work around a problem in UE 5.1 where negatively-scaled meshes
// don't work correctly for collision.
// See https://github.com/CesiumGS/cesium-unreal/pull/1126
// Note that this matrix is its own inverse.

constexpr glm::dmat4 yInvertMatrix = {
    1.0,
    0.0,
    0.0,
    0.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    0.0,
    0.0,
    1.0};

std::string getPrimitiveName(
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const CesiumGltf::MeshPrimitive& primitive) {
  std::string name = "glTF";

  const auto urlIt = model.extras.find("Cesium3DTiles_TileUrl");
  if (urlIt != model.extras.end()) {
    name = urlIt->second.getStringOrDefault("glTF");
    name = constrainLength(name, 256);
  }

  const auto meshIt = std::find_if(
      model.meshes.begin(),
      model.meshes.end(),
      [&mesh](const CesiumGltf::Mesh& candidate) {
        return &candidate == &mesh;
      });
  if (meshIt != model.meshes.end()) {
    int64_t meshIndex = meshIt - model.meshes.begin();
    name += " mesh " + std::to_string(meshIndex);
  }

  const auto primitiveIt = std::find_if(
      mesh.primitives.begin(),
      mesh.primitives.end(),
      [&primitive](const CesiumGltf::MeshPrimitive& candidate) {
        return &candidate == &primitive;
      });
  if (primitiveIt != mesh.primitives.end()) {
    int64_t primitiveIndex = primitiveIt - mesh.primitives.begin();
    name += " primitive " + std::to_string(primitiveIndex);
  }
  return name;
}

/// Helper used to log only once per unsupported primitive mode.
struct PrimitiveModeLogger {
  std::array<
      std::atomic_bool,
      (size_t)CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN + 1>
      alreadyLogged;

  PrimitiveModeLogger()
      : alreadyLogged{
            {{false}, {false}, {false}, {false}, {false}, {false}, {false}}} {}

  inline void OnUnsupportedMode(int32_t primMode) {
    bool bPrintLog = false;
    if (primMode < 0 || primMode >= (int32_t)alreadyLogged.size()) {
      ensureMsgf(false, TEXT("Unknown primitive mode %d!"), primMode);
      bPrintLog = true;
    } else if (!alreadyLogged[(size_t)primMode].exchange(true)) {
      bPrintLog = true;
    }
    if (bPrintLog) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Primitive mode %d is not supported"),
          primMode);
    }
  }
};
static PrimitiveModeLogger UnsupportedPrimitiveLogger;

template <class TIndexAccessor>
TArray<uint32>
getIndices(const TIndexAccessor& indicesView, int32 primitiveMode) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyIndices)
  TArray<uint32> indices;

  switch (primitiveMode) {
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
    // The TRIANGLE_STRIP primitive mode cannot be enabled without creating a
    // custom render proxy, so the geometry must be emulated through separate
    // triangles.
    indices.SetNum(
        static_cast<TArray<uint32>::SizeType>(3 * (indicesView.size() - 2)));
    for (int32 i = 0; i < indicesView.size() - 2; ++i) {
      if (i % 2) {
        indices[3 * i] = indicesView[i];
        indices[3 * i + 1] = indicesView[i + 2];
        indices[3 * i + 2] = indicesView[i + 1];
      } else {
        indices[3 * i] = indicesView[i];
        indices[3 * i + 1] = indicesView[i + 1];
        indices[3 * i + 2] = indicesView[i + 2];
      }
    }
    break;
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
    // The TRIANGLE_FAN primitive mode cannot be enabled without creating a
    // custom render proxy, so the geometry must be emulated through separate
    // triangles.
    indices.SetNum(
        static_cast<TArray<uint32>::SizeType>(3 * (indicesView.size() - 2)));
    for (int32 i = 2, j = 0; i < indicesView.size(); ++i, j += 3) {
      indices[j] = indicesView[0];
      indices[j + 1] = indicesView[i - 1];
      indices[j + 2] = indicesView[i];
    }
    break;
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLES:
  case CesiumGltf::MeshPrimitive::Mode::POINTS:
  default:
    indices.SetNum(static_cast<TArray<uint32>::SizeType>(indicesView.size()));
    for (int32 i = 0; i < indicesView.size(); ++i) {
      indices[i] = indicesView[i];
    }
    break;
  }

  return indices;
}
} // namespace

template <class TIndexAccessor>
static void loadPrimitive(
    LoadedPrimitiveResult& primitiveResult,
    const glm::dmat4x4& transform,
    const CreatePrimitiveOptions& options,
    const CesiumGltf::Accessor& positionAccessor,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const TIndexAccessor& indicesView,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadPrimitive<T>)

  CesiumGltf::Model& model =
      *options.pMeshOptions->pNodeOptions->pModelOptions->pModel;
  CesiumGltf::Mesh& mesh = model.meshes[options.pMeshOptions->meshIndex];
  CesiumGltf::MeshPrimitive& primitive =
      mesh.primitives[options.primitiveIndex];

  switch (primitive.mode) {
  case CesiumGltf::MeshPrimitive::Mode::POINTS:
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLES:
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP:
  case CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN:
    break;
  default:
    // TODO: add support for other primitive types.
    UnsupportedPrimitiveLogger.OnUnsupportedMode(primitive.mode);
    return;
  }

  const std::string name = getPrimitiveName(model, mesh, primitive);
  primitiveResult.name = name;

  if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("%s: Invalid position buffer"),
        UTF8_TO_TCHAR(name.c_str()));
    return;
  }

  if constexpr (IsAccessorView<TIndexAccessor>::value) {
    if (indicesView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s: Invalid indices buffer"),
          UTF8_TO_TCHAR(name.c_str()));
      return;
    }
  }

  auto normalAccessorIt =
      primitive.attributes.find(CesiumGltf::VertexAttributeSemantics::NORMAL);
  CesiumGltf::AccessorView<FVector3f> normalAccessor;
  bool hasNormals = false;
  if (normalAccessorIt != primitive.attributes.end()) {
    int normalAccessorID = normalAccessorIt->second;
    normalAccessor =
        CesiumGltf::AccessorView<FVector3f>(model, normalAccessorID);
    hasNormals =
        normalAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;
    if (!hasNormals) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "%s: Invalid normal buffer. Flat normals will be auto-generated instead."),
          UTF8_TO_TCHAR(name.c_str()));
    }
  }

  int materialID = primitive.material;
  const CesiumGltf::Material& material =
      materialID >= 0 && materialID < model.materials.size()
          ? model.materials[materialID]
          : defaultMaterial;

  primitiveResult.materialIndex = materialID;

  primitiveResult.isUnlit =
      material.hasExtension<CesiumGltf::ExtensionKhrMaterialsUnlit>() &&
      !options.pMeshOptions->pNodeOptions->pModelOptions
           ->ignoreKhrMaterialsUnlit;

  // We can't calculate flat normals for points or lines, so we have to force
  // them to be unlit if no normals are specified. Otherwise this causes a
  // crash when attempting to calculate flat normals.
  bool isTriangles =
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLES ||
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLE_FAN ||
      primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP;

  if (!isTriangles && !hasNormals) {
    primitiveResult.isUnlit = true;
  }

  const CesiumGltf::MaterialPBRMetallicRoughness& pbrMetallicRoughness =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  bool hasNormalMap = material.normalTexture.has_value();
  if (hasNormalMap) {
    const CesiumGltf::Texture* pTexture = CesiumGltf::Model::getSafe(
        &model.textures,
        material.normalTexture->index);
    hasNormalMap =
        pTexture != nullptr &&
        CesiumGltf::Model::getSafe(&model.images, pTexture->source) != nullptr;
  }

  bool needsTangents =
      hasNormalMap ||
      options.pMeshOptions->pNodeOptions->pModelOptions->alwaysIncludeTangents;

  bool hasTangents = false;
  auto tangentAccessorIt =
      primitive.attributes.find(CesiumGltf::VertexAttributeSemantics::TANGENT);
  CesiumGltf::AccessorView<FVector4f> tangentAccessor;
  if (tangentAccessorIt != primitive.attributes.end()) {
    int tangentAccessorID = tangentAccessorIt->second;
    tangentAccessor =
        CesiumGltf::AccessorView<FVector4f>(model, tangentAccessorID);
    hasTangents =
        tangentAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;
    if (!hasTangents) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s: Invalid tangent buffer."),
          UTF8_TO_TCHAR(name.c_str()));
    }
  }

  applyWaterMask(model, primitive, primitiveResult);

  // The water effect works by animating the normal, and the normal is
  // expressed in tangent space. So if we have water, we need tangents.
  if (primitiveResult.onlyWater || primitiveResult.waterMaskTexture) {
    needsTangents = true;
  }

  TUniquePtr<FStaticMeshRenderData> RenderData =
      MakeUnique<FStaticMeshRenderData>();
  RenderData->AllocateLODResources(1);

  FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ComputeAABB)

    const std::vector<double>& min = positionAccessor.min;
    const std::vector<double>& max = positionAccessor.max;

    glm::dvec3 minPosition{std::numeric_limits<double>::max()};
    glm::dvec3 maxPosition{std::numeric_limits<double>::lowest()};

    if (min.size() != 3 || max.size() != 3) {
      for (int64_t i = 0; i < positionView.size(); ++i) {
        minPosition.x = glm::min<double>(minPosition.x, positionView[i].X);
        minPosition.y = glm::min<double>(minPosition.y, positionView[i].Y);
        minPosition.z = glm::min<double>(minPosition.z, positionView[i].Z);

        maxPosition.x = glm::max<double>(maxPosition.x, positionView[i].X);
        maxPosition.y = glm::max<double>(maxPosition.y, positionView[i].Y);
        maxPosition.z = glm::max<double>(maxPosition.z, positionView[i].Z);
      }
    } else {
      minPosition = glm::dvec3(min[0], min[1], min[2]);
      maxPosition = glm::dvec3(max[0], max[1], max[2]);
    }

    minPosition *= CesiumPrimitiveData::positionScaleFactor;
    maxPosition *= CesiumPrimitiveData::positionScaleFactor;

    primitiveResult.dimensions =
        glm::vec3(transform * glm::dvec4(maxPosition - minPosition, 0));

    FBox aaBox(
        FVector3d(minPosition.x, -minPosition.y, minPosition.z),
        FVector3d(maxPosition.x, -maxPosition.y, maxPosition.z));

    aaBox.GetCenterAndExtents(
        RenderData->Bounds.Origin,
        RenderData->Bounds.BoxExtent);
    RenderData->Bounds.SphereRadius = 0.0f;
  }

  TArray<uint32> indices = getIndices(indicesView, primitive.mode);

  // If we don't have normals, the gltf spec prescribes that the client
  // implementation must generate flat normals, which requires duplicating
  // vertices shared by multiple triangles. If we don't have tangents, but
  // need them, we need to use a tangent space generation algorithm which
  // requires duplicated vertices.
  bool normalsAreRequired = !primitiveResult.isUnlit;
  bool needToGenerateFlatNormals = normalsAreRequired && !hasNormals;
  bool needToGenerateTangents = needsTangents && !hasTangents;
  bool duplicateVertices = needToGenerateFlatNormals || needToGenerateTangents;
  duplicateVertices = duplicateVertices &&
                      primitive.mode != CesiumGltf::MeshPrimitive::Mode::POINTS;

  uint32 numVertices =
      duplicateVertices ? uint32(indices.Num()) : uint32(positionView.size());

  FPositionVertexBuffer& positionBuffer =
      LODResources.VertexBuffers.PositionVertexBuffer;
  positionBuffer.Init(numVertices, false);

  {
    if (duplicateVertices) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyDuplicatedPositions)
      for (uint32 i = 0; i < numVertices; ++i) {
        uint32 vertexIndex = indices[i];
        const FVector3f& value = positionView[vertexIndex];
        FVector3f& position = positionBuffer.VertexPosition(i);
        position.X = value.X * CesiumPrimitiveData::positionScaleFactor;
        position.Y = -value.Y * CesiumPrimitiveData::positionScaleFactor;
        position.Z = value.Z * CesiumPrimitiveData::positionScaleFactor;
        RenderData->Bounds.SphereRadius = FMath::Max(
            (FVector(position) - RenderData->Bounds.Origin).Size(),
            RenderData->Bounds.SphereRadius);
      }
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyPositions)
      for (uint32 i = 0; i < numVertices; ++i) {
        const FVector3f& value = positionView[i];
        FVector3f& position = positionBuffer.VertexPosition(i);
        position.X = value.X * CesiumPrimitiveData::positionScaleFactor;
        position.Y = -value.Y * CesiumPrimitiveData::positionScaleFactor;
        position.Z = value.Z * CesiumPrimitiveData::positionScaleFactor;
        RenderData->Bounds.SphereRadius = FMath::Max(
            (FVector(position) - RenderData->Bounds.Origin).Size(),
            RenderData->Bounds.SphereRadius);
      }
    }
  }

  auto colorAccessorIt = primitive.attributes.find(
      CesiumGltf::VertexAttributeSemantics::COLOR_n[0]);
  if (colorAccessorIt != primitive.attributes.end()) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyVertexColors)
    LODResources.VertexBuffers.ColorVertexBuffer.Init(numVertices, false);
    LODResources.bHasColorVertexData = createAccessorView(
        model,
        colorAccessorIt->second,
        ColorVisitor{
            duplicateVertices,
            LODResources.VertexBuffers.ColorVertexBuffer,
            indices});
  }

  // Encodes the `EXT_primitive_features` and `EXT_structural_metadata`
  // extensions on the primitive, if present. This must be done before material
  // textures are loaded, in case any of the material textures are also used for
  // features + metadata.
  loadPrimitiveFeaturesMetadata(primitiveResult, options, model, primitive);

  const CreateModelOptions& modelOptions =
      *options.pMeshOptions->pNodeOptions->pModelOptions;
  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::AccumulateTextureCoordinates)
    const LoadGltfResult::LoadedModelResult* pModelResult =
        options.pMeshOptions->pNodeOptions->pHalfConstructedModelResult;

    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    if (modelOptions.pFeaturesMetadataDescription) {
      accumulateFeaturesMetadataAccessors(
          model,
          primitive,
          *pModelResult,
          primitiveResult);
    } else if (modelOptions.pEncodedMetadataDescription_DEPRECATED) {
      accumulateFeaturesMetadataAccessors_DEPRECATED(
          model,
          primitive,
          *pModelResult,
          primitiveResult);
    }
    PRAGMA_ENABLE_DEPRECATION_WARNINGS

    accumulateMaterialAndOverlayTextureCoordinates(
        model,
        primitive,
        material,
        pbrMetallicRoughness,
        primitiveResult);
  }

  // We need to copy the texture coordinates associated with each texture (if
  // any) into the appropriate UVs slot.
  std::unordered_map<int32_t, uint32_t>& texCoordMap =
      primitiveResult.GltfToUnrealTexCoordMap;

  uint32 numberOfTextureCoordinates =
      texCoordMap.size() == 0 ? 1 : uint32(texCoordMap.size());

  FStaticMeshVertexBuffer& vertexBuffer =
      LODResources.VertexBuffers.StaticMeshVertexBuffer;
  // Set to full precision (32-bit) UVs. This is especially important for
  // metadata because integer feature IDs can and will lose meaningful
  // precision when using 16-bit floats.
  vertexBuffer.SetUseFullPrecisionUVs(true);
  vertexBuffer.Init(numVertices, numberOfTextureCoordinates, false);

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadTextures)
    primitiveResult.baseColorTexture =
        loadTexture(model, pbrMetallicRoughness.baseColorTexture, true);
    primitiveResult.metallicRoughnessTexture = loadTexture(
        model,
        pbrMetallicRoughness.metallicRoughnessTexture,
        false);
    primitiveResult.normalTexture =
        loadTexture(model, material.normalTexture, false);
    primitiveResult.occlusionTexture =
        loadTexture(model, material.occlusionTexture, false);
    primitiveResult.emissiveTexture =
        loadTexture(model, material.emissiveTexture, true);
  }

  populateUnrealTexCoords(
      model,
      primitive,
      modelOptions,
      vertexBuffer,
      indices,
      duplicateVertices,
      primitiveResult);

  double scale = 1.0 / CesiumPrimitiveData::positionScaleFactor;
  glm::dmat4 scaleMatrix = glm::dmat4(
      glm::dvec4(scale, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, scale, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, scale, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));

  // TangentX: Tangent
  // TangentY: Bi-tangent
  // TangentZ: Normal

  if (hasNormals) {
    if (duplicateVertices) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyNormalsForDuplicatedVertices)
      for (int i = 0; i < indices.Num(); ++i) {
        uint32 vertexIndex = indices[i];
        const FVector3f& normal = normalAccessor[vertexIndex];

        vertexBuffer.SetVertexTangents(
            i,
            FVector3f(0.0f, 0.0f, 0.0f),
            FVector3f(0.0f, 0.0f, 0.0f),
            FVector3f(normal.X, -normal.Y, normal.Z));
      }
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyNormals)
      for (uint32 i = 0; i < numVertices; ++i) {
        const FVector3f& normal = normalAccessor[i];

        vertexBuffer.SetVertexTangents(
            i,
            FVector3f(0.0f, 0.0f, 0.0f),
            FVector3f(0.0f, 0.0f, 0.0f),
            FVector3f(normal.X, -normal.Y, normal.Z));
      }
    }
  } else {
    if (primitiveResult.isUnlit) {
      setUnlitNormals(
          LODResources.VertexBuffers,
          ellipsoid,
          transform * yInvertMatrix * scaleMatrix);
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ComputeFlatNormals)
      computeFlatNormals(LODResources.VertexBuffers);
    }
  }

  if (hasTangents) {
    if (duplicateVertices) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyTangentsForDuplicatedVertices)
      for (int i = 0; i < indices.Num(); ++i) {
        uint32 vertexIndex = indices[i];
        const FVector4f& tangent = tangentAccessor[vertexIndex];
        FVector3f tangentZ = vertexBuffer.VertexTangentZ(i);
        FVector3f tangentX = FVector3f(tangent.X, -tangent.Y, tangent.Z);
        FVector3f tangentY =
            FVector3f::CrossProduct(tangentZ, tangentX) * tangent.W;
        vertexBuffer.SetVertexTangents(i, tangentX, tangentY, tangentZ);
      }
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyTangents)
      for (uint32 i = 0; i < numVertices; ++i) {
        const FVector4f& tangent = tangentAccessor[i];
        FVector3f tangentZ = vertexBuffer.VertexTangentZ(i);
        FVector3f tangentX = FVector3f(tangent.X, -tangent.Y, tangent.Z);
        FVector3f tangentY =
            FVector3f::CrossProduct(tangentZ, tangentX) * tangent.W;
        vertexBuffer.SetVertexTangents(i, tangentX, tangentY, tangentZ);
      }
    }
  }

  if (needsTangents && !hasTangents) {
    // Use mikktspace to calculate the tangents.
    // Note that this assumes normals and UVs are already populated.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ComputeTangents)
    computeTangentSpace(LODResources.VertexBuffers);
  }

  FStaticMeshSectionArray& Sections = LODResources.Sections;
  FStaticMeshSection& section = Sections.AddDefaulted_GetRef();
  // This will be ignored if the primitive contains points.
  section.NumTriangles = indices.Num() / 3;
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = numVertices - 1;
  section.bEnableCollision =
      primitive.mode != CesiumGltf::MeshPrimitive::Mode::POINTS;
  section.bCastShadow = true;
  section.MaterialIndex = 0;

  if (duplicateVertices) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ReverseWindingOrder)
    for (int32 i = 0; i < indices.Num(); i++) {
      indices[i] = i;
    }
  }

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetIndices)
    LODResources.IndexBuffer.SetIndices(
        indices,
        numVertices >= std::numeric_limits<uint16>::max()
            ? EIndexBufferStride::Type::Force32Bit
            : EIndexBufferStride::Type::Force16Bit);
  }

  LODResources.bHasDepthOnlyIndices = false;
  LODResources.bHasReversedIndices = false;
  LODResources.bHasReversedDepthOnlyIndices = false;

#if ENGINE_VERSION_5_5_OR_HIGHER
  // UE 5.5 requires that we do this in order to avoid a crash when ray
  // tracing is enabled.
  if (primitive.mode != CesiumGltf::MeshPrimitive::Mode::POINTS) {
    // UE 5.5 requires that we do this in order to avoid a crash when ray
    // tracing is enabled.
    RenderData->InitializeRayTracingRepresentationFromRenderingLODs();
  }
#endif

  primitiveResult.meshIndex = options.pMeshOptions->meshIndex;
  primitiveResult.primitiveIndex = options.primitiveIndex;
  primitiveResult.RenderData = std::move(RenderData);
  primitiveResult.pCollisionMesh = nullptr;

  primitiveResult.transform = transform * yInvertMatrix * scaleMatrix;

  if (primitive.mode != CesiumGltf::MeshPrimitive::Mode::POINTS &&
      options.pMeshOptions->pNodeOptions->pModelOptions->createPhysicsMeshes) {
    if (numVertices != 0 && indices.Num() != 0) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ChaosCook)
      primitiveResult.pCollisionMesh =
          numVertices < TNumericLimits<uint16>::Max()
              ? BuildChaosTriangleMeshes<uint16>(
                    LODResources.VertexBuffers.PositionVertexBuffer,
                    indices)
              : BuildChaosTriangleMeshes<int32>(
                    LODResources.VertexBuffers.PositionVertexBuffer,
                    indices);
    }
  }
}

static void loadIndexedPrimitive(
    LoadedPrimitiveResult& primitiveResult,
    const glm::dmat4x4& transform,
    const CreatePrimitiveOptions& options,
    const CesiumGltf::Accessor& positionAccessor,
    const CesiumGltf::AccessorView<FVector3f>& positionView,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const CesiumGltf::Model& model =
      *options.pMeshOptions->pNodeOptions->pModelOptions->pModel;
  const CesiumGltf::MeshPrimitive& primitive =
      model.meshes[options.pMeshOptions->meshIndex]
          .primitives[options.primitiveIndex];

  const CesiumGltf::Accessor& indexAccessorGltf =
      model.accessors[primitive.indices];
  if (indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE) {
    CesiumGltf::AccessorView<uint8_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        primitiveResult,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor,
        ellipsoid);
    primitiveResult.IndexAccessor = indexAccessor;
  } else if (
      indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT) {
    CesiumGltf::AccessorView<uint16_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        primitiveResult,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor,
        ellipsoid);
    primitiveResult.IndexAccessor = indexAccessor;
  } else if (
      indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::UNSIGNED_INT) {
    CesiumGltf::AccessorView<uint32_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        primitiveResult,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor,
        ellipsoid);
    primitiveResult.IndexAccessor = indexAccessor;
  } else {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Ignoring a glTF primitive because the componentType (%d) of its indices is not supported."),
        indexAccessorGltf.componentType);
  }
}

static void loadPrimitive(
    LoadedPrimitiveResult& result,
    const glm::dmat4x4& transform,
    const CreatePrimitiveOptions& options,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadPrimitive)

  const CesiumGltf::Model& model =
      *options.pMeshOptions->pNodeOptions->pModelOptions->pModel;
  const CesiumGltf::MeshPrimitive& primitive =
      model.meshes[options.pMeshOptions->meshIndex]
          .primitives[options.primitiveIndex];

  auto positionAccessorIt =
      primitive.attributes.find(CesiumGltf::VertexAttributeSemantics::POSITION);
  if (positionAccessorIt == primitive.attributes.end()) {
    // This primitive doesn't have a POSITION semantic, ignore it.
    return;
  }

  int positionAccessorID = positionAccessorIt->second;
  const CesiumGltf::Accessor* pPositionAccessor =
      CesiumGltf::Model::getSafe(&model.accessors, positionAccessorID);
  if (!pPositionAccessor) {
    // Position accessor does not exist, so ignore this primitive.
    return;
  }

  CesiumGltf::AccessorView<FVector3f> positionView(model, *pPositionAccessor);

  if (primitive.indices < 0 || primitive.indices >= model.accessors.size()) {
    std::vector<uint32_t> syntheticIndexBuffer(positionView.size());
    syntheticIndexBuffer.resize(positionView.size());
    for (uint32_t i = 0; i < positionView.size(); ++i) {
      syntheticIndexBuffer[i] = i;
    }
    loadPrimitive(
        result,
        transform,
        options,
        *pPositionAccessor,
        positionView,
        syntheticIndexBuffer,
        ellipsoid);
  } else {
    loadIndexedPrimitive(
        result,
        transform,
        options,
        *pPositionAccessor,
        positionView,
        ellipsoid);
  }
  result.PositionAccessor = std::move(positionView);
}

static void loadMesh(
    std::optional<LoadedMeshResult>& result,
    const glm::dmat4x4& transform,
    CreateMeshOptions& options,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadMesh)

  CesiumGltf::Model& model = *options.pNodeOptions->pModelOptions->pModel;
  CesiumGltf::Mesh& mesh = model.meshes[options.meshIndex];

  result = LoadedMeshResult();
  result->primitiveResults.reserve(mesh.primitives.size());
  for (size_t i = 0; i < mesh.primitives.size(); i++) {
    CreatePrimitiveOptions primitiveOptions = {&options, &*result, i};
    auto& primitiveResult = result->primitiveResults.emplace_back();
    loadPrimitive(primitiveResult, transform, primitiveOptions, ellipsoid);

    // if it doesn't have render data, then it can't be loaded
    if (!primitiveResult.RenderData) {
      result->primitiveResults.pop_back();
    }
  }
}

// Helpers for different instancing rotation formats

namespace {
template <typename T> struct is_float_quat : std::false_type {};

template <>
struct is_float_quat<CesiumGltf::AccessorTypes::VEC4<float>> : std::true_type {
};

template <typename T> struct is_int_quat : std::false_type {};

template <typename T>
struct is_int_quat<CesiumGltf::AccessorTypes::VEC4<T>>
    : std::conjunction<std::is_integral<T>, std::is_signed<T>> {};

template <typename T>
inline constexpr bool is_float_quat_v = is_float_quat<T>::value;

template <typename T>
inline constexpr bool is_int_quat_v = is_int_quat<T>::value;
} // namespace

static void loadInstancingData(
    const CesiumGltf::Model& model,
    const CesiumGltf::Node& node,
    LoadedNodeResult& result,
    const CesiumGltf::ExtensionExtMeshGpuInstancing* pGpuInstancing,
    const CesiumGltf::ExtensionExtInstanceFeatures* pInstanceFeatures) {
  auto getInstanceAccessor =
      [&](const char* name) -> const CesiumGltf::Accessor* {
    if (auto accessorItr = pGpuInstancing->attributes.find(name);
        accessorItr != pGpuInstancing->attributes.end()) {
      return CesiumGltf::Model::getSafe(&model.accessors, accessorItr->second);
    }
    return nullptr;
  };
  const CesiumGltf::Accessor* translations = getInstanceAccessor("TRANSLATION");
  const CesiumGltf::Accessor* rotations = getInstanceAccessor("ROTATION");
  const CesiumGltf::Accessor* scales = getInstanceAccessor("SCALE");

  int64_t count = 0;
  if (translations) {
    count = translations->count;
  }
  if (rotations) {
    if (count == 0) {
      count = rotations->count;
    } else if (count != rotations->count) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("instance rotation count %d not consistent with %d"),
          rotations->count,
          count);
      return;
    }
  }
  if (scales) {
    if (count == 0) {
      count = scales->count;
    } else if (count != scales->count) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("instance scale count %d not consistent with %d"),
          scales->count,
          count);
      return;
    }
  }
  if (count == 0) {
    UE_LOG(LogCesium, Warning, TEXT("No valid instance data"));
    return;
  }
  // The glTF instance transforms need to be transformed into the local
  // coordinate system of the Unreal static mesh i.e., Unreals' left-handed
  // system. Another way to think about it is that the geometry, which is
  // stored in the Unreal system, must be transformed to glTF, have the
  // instance transform applied, and then be transformed back to Unreal. It's
  // tempting to do this by trying some manipulation of the individual glTF
  // instance operations, but that general approach has always ended in
  // tears. Better to formally multiply out the matrices and be assured that the
  // operation is correct.
  std::vector<glm::dmat4> instanceTransforms(count, glm::dmat4(1.0));

  // Note: the glm functions translate() and scale() post-multiply the matrix
  // argument by the new transform. E.g., translate() does *not* translate the
  // matrix.
  if (translations) {
    CesiumGltf::AccessorView<glm::fvec3> translationAccessor(
        model,
        *translations);
    if (translationAccessor.status() == CesiumGltf::AccessorViewStatus::Valid) {
      for (int64_t i = 0; i < count; ++i) {
        glm::dvec3 translation(translationAccessor[i]);
        instanceTransforms[i] = glm::translate(
            instanceTransforms[i],
            translation * CesiumPrimitiveData::positionScaleFactor);
      }
    }
  } else {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Invalid accessor for instance translations"));
  }
  if (rotations) {
    createAccessorView(model, *rotations, [&](auto&& quatView) -> void {
      using QuatType = decltype(quatView[0]);
      using ValueType = std::decay_t<QuatType>;
      if constexpr (is_float_quat_v<ValueType>) {
        for (int i = 0; i < count; ++i) {
          glm::dquat quat(
              quatView[i].value[3],
              quatView[i].value[0],
              quatView[i].value[1],
              quatView[i].value[2]);
          instanceTransforms[i] = instanceTransforms[i] * glm::mat4_cast(quat);
        }
      } else if constexpr (is_int_quat_v<ValueType>) {
        for (int64_t i = 0; i < count; ++i) {
          float val[4];
          for (int j = 0; j < 4; ++j) {
            val[j] = GltfNormalized(quatView[i].value[j]);
          }
          glm::dquat quat(val[3], val[0], val[1], val[2]);
          instanceTransforms[i] = instanceTransforms[i] * glm::mat4_cast(quat);
        }
      }
    });
  }
  if (scales) {
    CesiumGltf::AccessorView<glm::fvec3> scaleAccessor(model, *scales);
    for (int64_t i = 0; i < count; ++i) {
      glm::dvec3 scaleFactors(scaleAccessor[i]);
      instanceTransforms[i] = glm::scale(instanceTransforms[i], scaleFactors);
    }
  } else {
    UE_LOG(LogCesium, Warning, TEXT("Invalid accessor for instance scales"));
  }
  result.InstanceTransforms.resize(count);
  for (int64_t i = 0; i < count; ++i) {
    glm::dmat4 unrealMat =
        yInvertMatrix * instanceTransforms[i] * yInvertMatrix;
    result.InstanceTransforms[i] = VecMath::createTransform(unrealMat);
  }
  if (pInstanceFeatures) {
    result.pInstanceFeatures =
        MakeShared<FCesiumPrimitiveFeatures>(model, node, *pInstanceFeatures);
  }
}

static void loadNode(
    std::vector<LoadedNodeResult>& loadNodeResults,
    const glm::dmat4x4& transform,
    CreateNodeOptions& options,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadNode)

  static constexpr std::array<double, 16> identityMatrix = {
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0};

  CesiumGltf::Model& model = *options.pModelOptions->pModel;
  const CesiumGltf::Node& node = *options.pNode;

  LoadedNodeResult& result = loadNodeResults.emplace_back();

  glm::dmat4x4 nodeTransform = transform;

  const std::vector<double>& matrix = node.matrix;
  bool isIdentityMatrix = false;
  if (matrix.size() == 16) {
    isIdentityMatrix =
        std::equal(matrix.begin(), matrix.end(), identityMatrix.begin());
  }

  if (matrix.size() == 16 && !isIdentityMatrix) {
    glm::dmat4x4 nodeTransformGltf(
        glm::dvec4(matrix[0], matrix[1], matrix[2], matrix[3]),
        glm::dvec4(matrix[4], matrix[5], matrix[6], matrix[7]),
        glm::dvec4(matrix[8], matrix[9], matrix[10], matrix[11]),
        glm::dvec4(matrix[12], matrix[13], matrix[14], matrix[15]));

    nodeTransform = nodeTransform * nodeTransformGltf;
  } else {
    glm::dmat4 translation(1.0);
    if (node.translation.size() == 3) {
      translation[3] = glm::dvec4(
          node.translation[0],
          node.translation[1],
          node.translation[2],
          1.0);
    }

    glm::dquat rotationQuat(1.0, 0.0, 0.0, 0.0);
    if (node.rotation.size() == 4) {
      rotationQuat[0] = node.rotation[0];
      rotationQuat[1] = node.rotation[1];
      rotationQuat[2] = node.rotation[2];
      rotationQuat[3] = node.rotation[3];
    }

    glm::dmat4 scale(1.0);
    if (node.scale.size() == 3) {
      scale[0].x = node.scale[0];
      scale[1].y = node.scale[1];
      scale[2].z = node.scale[2];
    }

    nodeTransform =
        nodeTransform * translation * glm::dmat4(rotationQuat) * scale;
  }

  int meshId = node.mesh;
  if (meshId >= 0 && meshId < model.meshes.size()) {
    if (const auto* pGpuInstancingExtension =
            node.getExtension<CesiumGltf::ExtensionExtMeshGpuInstancing>()) {
      loadInstancingData(
          model,
          node,
          result,
          pGpuInstancingExtension,
          node.getExtension<CesiumGltf::ExtensionExtInstanceFeatures>());
    }
    CreateMeshOptions meshOptions = {&options, &result, meshId};
    loadMesh(result.meshResult, nodeTransform, meshOptions, ellipsoid);
  }

  for (int childNodeId : node.children) {
    if (childNodeId >= 0 && childNodeId < model.nodes.size()) {
      CreateNodeOptions childNodeOptions = {
          options.pModelOptions,
          options.pHalfConstructedModelResult,
          &model.nodes[childNodeId]};
      loadNode(loadNodeResults, nodeTransform, childNodeOptions, ellipsoid);
    }
  }
}

namespace {
/**
 * @brief Apply the transform so that the up-axis of the given model is the
 * Z-axis.
 *
 * By default, the up-axis of a glTF model will the the Y-axis.
 *
 * If the tileset that contained the model had the `asset.gltfUpAxis` string
 * property, then the information about the up-axis has been stored in as a
 * number property called `gltfUpAxis` in the `extras` of the given model.
 *
 * Depending on whether this value is CesiumGeometry::Axis::X, Y, or Z,
 * the given matrix will be multiplied with a matrix that converts the
 * respective axis to be the Z-axis, as required by the 3D Tiles standard.
 *
 * @param model The glTF model
 * @param rootTransform The matrix that will be multiplied with the transform
 */
void applyGltfUpAxisTransform(
    const CesiumGltf::Model& model,
    glm::dmat4x4& rootTransform) {

  auto gltfUpAxisIt = model.extras.find("gltfUpAxis");
  if (gltfUpAxisIt == model.extras.end()) {
    // The default up-axis of glTF is the Y-axis, and no other
    // up-axis was specified. Transform the Y-axis to the Z-axis,
    // to match the 3D Tiles specification
    rootTransform *= CesiumGeometry::Transforms::Y_UP_TO_Z_UP;
    return;
  }
  const CesiumUtility::JsonValue& gltfUpAxis = gltfUpAxisIt->second;
  int gltfUpAxisValue = static_cast<int>(gltfUpAxis.getSafeNumberOrDefault(1));
  if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::X)) {
    rootTransform *= CesiumGeometry::Transforms::X_UP_TO_Z_UP;
  } else if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::Y)) {
    rootTransform *= CesiumGeometry::Transforms::Y_UP_TO_Z_UP;
  } else if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::Z)) {
    // No transform required
  } else {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Ignoring unknown gltfUpAxis value: {}"),
        gltfUpAxisValue);
  }
}

} // namespace

static void loadModelMetadata(
    LoadedModelResult& result,
    const CreateModelOptions& options) {
  CesiumGltf::Model& model = *options.pModel;

  CesiumGltf::ExtensionModelExtStructuralMetadata* pModelMetadata =
      model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  if (!pModelMetadata) {
    return;
  }

  model.forEachPrimitiveInScene(
      model.scene,
      [pModelMetadata](
          CesiumGltf::Model& gltf,
          CesiumGltf::Node& /*node*/,
          CesiumGltf::Mesh& /*mesh*/,
          CesiumGltf::MeshPrimitive& primitive,
          const glm::dmat4& /*nodeTransform*/) {
        const CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata*
            pPrimitiveMetadata = primitive.getExtension<
                CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata>();
        if (!pPrimitiveMetadata) {
          return;
        }

        int32_t materialIndex = primitive.material;
        if (materialIndex < 0 || materialIndex >= gltf.materials.size()) {
          return;
        }

        const CesiumGltf::Material& material =
            gltf.materials[primitive.material];

        for (const auto& propertyTextureIndex :
             pPrimitiveMetadata->propertyTextures) {
          if (propertyTextureIndex < 0 ||
              static_cast<size_t>(propertyTextureIndex) >=
                  pModelMetadata->propertyTextures.size()) {
            continue;
          }

          CesiumGltf::PropertyTexture& propertyTexture =
              pModelMetadata->propertyTextures[propertyTextureIndex];

          for (auto& propertyIt : propertyTexture.properties) {
            if (propertyIt.second.extras.find("makeImageCopy") !=
                propertyIt.second.extras.end()) {
              continue;
            }

            int32_t textureIndex = propertyIt.second.index;
            if (textureIndex < 0 || textureIndex > gltf.textures.size()) {
              continue;
            }

            const CesiumGltf::Texture& texture = gltf.textures[textureIndex];
            if (texture.source < 0 || texture.source >= gltf.images.size()) {
              continue;
            }

            if (hasMaterialTextureConflicts(gltf, material, texture.source)) {
              // Add a flag in the extras to indicate a copy should be made.
              // This is checked for in FCesiumPropertyTexture.
              propertyIt.second.extras.insert({"makeImageCopy", true});
            }
          }
        }
      });

  result.Metadata = FCesiumModelMetadata(model, *pModelMetadata);

  const FCesiumFeaturesMetadataDescription* pFeaturesMetadataDescription =
      options.pFeaturesMetadataDescription;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  const FMetadataDescription* pMetadataDescription_DEPRECATED =
      options.pEncodedMetadataDescription_DEPRECATED;
  if (pFeaturesMetadataDescription) {
    result.EncodedMetadata =
        EncodedFeaturesMetadata::encodeModelMetadataAnyThreadPart(
            pFeaturesMetadataDescription->ModelMetadata,
            result.Metadata);
  } else if (pMetadataDescription_DEPRECATED) {
    result.EncodedMetadata_DEPRECATED =
        CesiumEncodedMetadataUtility::encodeMetadataAnyThreadPart(
            *pMetadataDescription_DEPRECATED,
            result.Metadata);
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

static CesiumAsync::Future<UCesiumGltfComponent::CreateOffGameThreadResult>
loadModelAnyThreadPart(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const glm::dmat4x4& transform,
    CreateModelOptions&& options,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadModelAnyThreadPart)

  return CesiumGltfTextures::createInWorkerThread(asyncSystem, *options.pModel)
      .thenInWorkerThread(
          [transform, ellipsoid, options = std::move(options)]() mutable
              -> UCesiumGltfComponent::CreateOffGameThreadResult {
            auto pHalf = MakeUnique<HalfConstructedReal>();

            loadModelMetadata(pHalf->loadModelResult, options);

            glm::dmat4x4 rootTransform = transform;

            CesiumGltf::Model& model = *options.pModel;

            {
              rootTransform = CesiumGltfContent::GltfUtilities::applyRtcCenter(
                  model,
                  rootTransform);
              applyGltfUpAxisTransform(model, rootTransform);
            }

            if (model.scene >= 0 && model.scene < model.scenes.size()) {
              // Show the default scene
              const CesiumGltf::Scene& defaultScene = model.scenes[model.scene];
              for (int nodeId : defaultScene.nodes) {
                CreateNodeOptions nodeOptions = {
                    &options,
                    &pHalf->loadModelResult,
                    &model.nodes[nodeId]};
                loadNode(
                    pHalf->loadModelResult.nodeResults,
                    rootTransform,
                    nodeOptions,
                    ellipsoid);
              }
            } else if (model.scenes.size() > 0) {
              // There's no default, so show the first scene
              const CesiumGltf::Scene& defaultScene = model.scenes[0];
              for (int nodeId : defaultScene.nodes) {
                CreateNodeOptions nodeOptions = {
                    &options,
                    &pHalf->loadModelResult,
                    &model.nodes[nodeId]};
                loadNode(
                    pHalf->loadModelResult.nodeResults,
                    rootTransform,
                    nodeOptions,
                    ellipsoid);
              }
            } else if (model.nodes.size() > 0) {
              // No scenes at all, use the first node as the root node.
              CreateNodeOptions nodeOptions = {
                  &options,
                  &pHalf->loadModelResult,
                  &model.nodes[0]};
              loadNode(
                  pHalf->loadModelResult.nodeResults,
                  rootTransform,
                  nodeOptions,
                  ellipsoid);
            } else if (model.meshes.size() > 0) {
              // No nodes either, show all the meshes.
              for (size_t i = 0; i < model.meshes.size(); i++) {
                CreateNodeOptions dummyNodeOptions = {
                    &options,
                    &pHalf->loadModelResult,
                    nullptr};
                LoadedNodeResult& dummyNodeResult =
                    pHalf->loadModelResult.nodeResults.emplace_back();
                CreateMeshOptions meshOptions = {
                    &dummyNodeOptions,
                    &dummyNodeResult,
                    i};
                loadMesh(
                    dummyNodeResult.meshResult,
                    rootTransform,
                    meshOptions,
                    ellipsoid);
              }
            }

            UCesiumGltfComponent::CreateOffGameThreadResult result;
            result.HalfConstructed = std::move(pHalf);
            result.TileLoadResult = std::move(options.tileLoadResult);

            return result;
          });
}

bool applyTexture(
    CesiumGltf::Model& model,
    UMaterialInstanceDynamic* pMaterial,
    const FMaterialParameterInfo& info,
    CesiumTextureUtility::LoadedTextureResult* pLoadedTexture) {
  CesiumUtility::IntrusivePointer<
      CesiumTextureUtility::ReferenceCountedUnrealTexture>
      pTexture = CesiumTextureUtility::loadTextureGameThreadPart(
          model,
          pLoadedTexture);
  if (!pTexture) {
    return false;
  }

  pMaterial->SetTextureParameterValueByInfo(info, pTexture->getUnrealTexture());

  return true;
}

#pragma region Material Parameter setters

static void SetGltfParameterValues(
    CesiumGltf::Model& model,
    LoadedPrimitiveResult& loadResult,
    const CesiumGltf::Material& material,
    const CesiumGltf::MaterialPBRMetallicRoughness& pbr,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  for (auto& textureCoordinateSet : loadResult.textureCoordinateParameters) {
    pMaterial->SetScalarParameterValueByInfo(
        FMaterialParameterInfo(
            UTF8_TO_TCHAR(textureCoordinateSet.first.c_str()),
            association,
            index),
        static_cast<float>(textureCoordinateSet.second));
  }

  if (pbr.baseColorFactor.size() > 3) {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("baseColorFactor", association, index),
        FLinearColor(
            pbr.baseColorFactor[0],
            pbr.baseColorFactor[1],
            pbr.baseColorFactor[2],
            pbr.baseColorFactor[3]));
  } else if (pbr.baseColorFactor.size() == 3) {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("baseColorFactor", association, index),
        FLinearColor(
            pbr.baseColorFactor[0],
            pbr.baseColorFactor[1],
            pbr.baseColorFactor[2],
            1.));
  } else {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("baseColorFactor", association, index),
        FLinearColor(1., 1., 1., 1.));
  }
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("metallicFactor", association, index),
      static_cast<float>(loadResult.isUnlit ? 0.0f : pbr.metallicFactor));
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("roughnessFactor", association, index),
      static_cast<float>(loadResult.isUnlit ? 1.0f : pbr.roughnessFactor));
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("opacityMask", association, index),
      1.0f);

  applyTexture(
      model,
      pMaterial,
      FMaterialParameterInfo("baseColorTexture", association, index),
      loadResult.baseColorTexture.Get());
  applyTexture(
      model,
      pMaterial,
      FMaterialParameterInfo("metallicRoughnessTexture", association, index),
      loadResult.metallicRoughnessTexture.Get());
  applyTexture(
      model,
      pMaterial,
      FMaterialParameterInfo("normalTexture", association, index),
      loadResult.normalTexture.Get());
  bool hasEmissiveTexture = applyTexture(
      model,
      pMaterial,
      FMaterialParameterInfo("emissiveTexture", association, index),
      loadResult.emissiveTexture.Get());
  applyTexture(
      model,
      pMaterial,
      FMaterialParameterInfo("occlusionTexture", association, index),
      loadResult.occlusionTexture.Get());

  CesiumGltf::KhrTextureTransform textureTransform;
  FLinearColor baseColorMetallicRoughnessRotation(0.0f, 1.0f, 0.0f, 1.0f);
  const CesiumGltf::ExtensionKhrTextureTransform* pBaseColorTextureTransform =
      pbr.baseColorTexture
          ? pbr.baseColorTexture
                ->getExtension<CesiumGltf::ExtensionKhrTextureTransform>()
          : nullptr;

  if (pBaseColorTextureTransform) {
    textureTransform =
        CesiumGltf::KhrTextureTransform(*pBaseColorTextureTransform);
    if (textureTransform.status() ==
        CesiumGltf::KhrTextureTransformStatus::Valid) {
      const glm::dvec2& scale = textureTransform.scale();
      const glm::dvec2& offset = textureTransform.offset();
      pMaterial->SetVectorParameterValueByInfo(
          FMaterialParameterInfo("baseColorScaleOffset", association, index),
          FLinearColor(scale[0], scale[1], offset[0], offset[1]));

      const glm::dvec2& rotationSineCosine =
          textureTransform.rotationSineCosine();
      baseColorMetallicRoughnessRotation.R = rotationSineCosine[0];
      baseColorMetallicRoughnessRotation.G = rotationSineCosine[1];
    }
  }

  const CesiumGltf::ExtensionKhrTextureTransform*
      pMetallicRoughnessTextureTransform =
          pbr.metallicRoughnessTexture
              ? pbr.metallicRoughnessTexture
                    ->getExtension<CesiumGltf::ExtensionKhrTextureTransform>()
              : nullptr;

  if (pMetallicRoughnessTextureTransform) {
    textureTransform =
        CesiumGltf::KhrTextureTransform(*pMetallicRoughnessTextureTransform);
    if (textureTransform.status() ==
        CesiumGltf::KhrTextureTransformStatus::Valid) {
      const glm::dvec2& scale = textureTransform.scale();
      const glm::dvec2& offset = textureTransform.offset();
      pMaterial->SetVectorParameterValueByInfo(
          FMaterialParameterInfo(
              "metallicRoughnessScaleOffset",
              association,
              index),
          FLinearColor(scale[0], scale[1], offset[0], offset[1]));

      const glm::dvec2& rotationSineCosine =
          textureTransform.rotationSineCosine();
      baseColorMetallicRoughnessRotation.B = rotationSineCosine[0];
      baseColorMetallicRoughnessRotation.A = rotationSineCosine[1];
    }
  }

  if (pBaseColorTextureTransform || pMetallicRoughnessTextureTransform) {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            "baseColorMetallicRoughnessRotation",
            association,
            index),
        baseColorMetallicRoughnessRotation);
  }

  FLinearColor emissiveNormalRotation(0.0f, 1.0f, 0.0f, 1.0f);

  const CesiumGltf::ExtensionKhrTextureTransform* pEmissiveTextureTransform =
      material.emissiveTexture
          ? material.emissiveTexture
                ->getExtension<CesiumGltf::ExtensionKhrTextureTransform>()
          : nullptr;

  if (pEmissiveTextureTransform) {
    textureTransform =
        CesiumGltf::KhrTextureTransform(*pEmissiveTextureTransform);
    const glm::dvec2& scale = textureTransform.scale();
    const glm::dvec2& offset = textureTransform.offset();
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("emissiveScaleOffset", association, index),
        FLinearColor(scale[0], scale[1], offset[0], offset[1]));

    const glm::dvec2& rotationSineCosine =
        textureTransform.rotationSineCosine();
    emissiveNormalRotation.R = rotationSineCosine[0];
    emissiveNormalRotation.G = rotationSineCosine[1];
  }

  const CesiumGltf::ExtensionKhrTextureTransform* pNormalTextureTransform =
      material.normalTexture
          ? material.normalTexture
                ->getExtension<CesiumGltf::ExtensionKhrTextureTransform>()
          : nullptr;

  if (pNormalTextureTransform) {
    textureTransform =
        CesiumGltf::KhrTextureTransform(*pNormalTextureTransform);
    const glm::dvec2& scale = textureTransform.scale();
    const glm::dvec2& offset = textureTransform.offset();
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("normalScaleOffset", association, index),
        FLinearColor(scale[0], scale[1], offset[0], offset[1]));
    const glm::dvec2& rotationSineCosine =
        textureTransform.rotationSineCosine();
    emissiveNormalRotation.B = rotationSineCosine[0];
    emissiveNormalRotation.A = rotationSineCosine[1];
  }

  if (pEmissiveTextureTransform || pNormalTextureTransform) {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("emissiveNormalRotation", association, index),
        emissiveNormalRotation);
  }

  const CesiumGltf::ExtensionKhrTextureTransform* pOcclusionTransform =
      material.occlusionTexture
          ? material.occlusionTexture
                ->getExtension<CesiumGltf::ExtensionKhrTextureTransform>()
          : nullptr;

  if (pOcclusionTransform) {
    textureTransform = CesiumGltf::KhrTextureTransform(*pOcclusionTransform);
    const glm::dvec2& scale = textureTransform.scale();
    const glm::dvec2& offset = textureTransform.offset();
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("occlusionScaleOffset", association, index),
        FLinearColor(scale[0], scale[1], offset[0], offset[1]));

    const glm::dvec2& rotationSineCosine =
        textureTransform.rotationSineCosine();
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("occlusionRotation", association, index),
        FLinearColor(
            float(rotationSineCosine[0]),
            float(rotationSineCosine[1]),
            0.0f,
            1.0f));
  }

  if (material.emissiveFactor.size() >= 3) {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("emissiveFactor", association, index),
        FVector(
            material.emissiveFactor[0],
            material.emissiveFactor[1],
            material.emissiveFactor[2]));
  } else if (hasEmissiveTexture) {
    // When we have an emissive texture but not a factor, we need to use a
    // factor of vec3(1.0). The default, vec3(0.0), would disable the emission
    // from the texture.
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("emissiveFactor", association, index),
        FVector(1.0f, 1.0f, 1.0f));
  }
}

void SetWaterParameterValues(
    CesiumGltf::Model& model,
    LoadedPrimitiveResult& loadResult,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("OnlyLand", association, index),
      static_cast<float>(loadResult.onlyLand));
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("OnlyWater", association, index),
      static_cast<float>(loadResult.onlyWater));

  if (!loadResult.onlyLand && !loadResult.onlyWater) {
    applyTexture(
        model,
        pMaterial,
        FMaterialParameterInfo("WaterMask", association, index),
        loadResult.waterMaskTexture.Get());
  }

  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo("WaterMaskTranslationScale", association, index),
      FVector(
          loadResult.waterMaskTranslationX,
          loadResult.waterMaskTranslationY,
          loadResult.waterMaskScale));
}

static void SetFeaturesMetadataParameterValues(
    const CesiumGltf::Model& model,
    UCesiumGltfComponent& gltfComponent,
    LoadedPrimitiveResult& loadResult,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  // This handles texture coordinate indices for both attribute feature ID
  // sets and property textures.
  for (const auto& textureCoordinateSet :
       loadResult.FeaturesMetadataTexCoordParameters) {
    pMaterial->SetScalarParameterValueByInfo(
        FMaterialParameterInfo(
            FName(textureCoordinateSet.Key),
            association,
            index),
        textureCoordinateSet.Value);
  }

  if (encodePrimitiveFeaturesGameThreadPart(loadResult.EncodedFeatures)) {
    for (EncodedFeaturesMetadata::EncodedFeatureIdSet& encodedFeatureIdSet :
         loadResult.EncodedFeatures.featureIdSets) {
      FString SafeName =
          EncodedFeaturesMetadata::createHlslSafeName(encodedFeatureIdSet.name);
      if (encodedFeatureIdSet.nullFeatureId) {
        pMaterial->SetScalarParameterValueByInfo(
            FMaterialParameterInfo(
                FName(
                    SafeName +
                    EncodedFeaturesMetadata::MaterialNullFeatureIdSuffix),
                association,
                index),
            static_cast<float>(*encodedFeatureIdSet.nullFeatureId));
      }

      if (encodedFeatureIdSet.texture) {
        EncodedFeaturesMetadata::SetFeatureIdTextureParameterValues(
            pMaterial,
            association,
            index,
            SafeName,
            *encodedFeatureIdSet.texture);
      }
    }

    for (const EncodedFeaturesMetadata::EncodedPropertyTexture&
             propertyTexture : gltfComponent.EncodedMetadata.propertyTextures) {
      EncodedFeaturesMetadata::SetPropertyTextureParameterValues(
          pMaterial,
          association,
          index,
          propertyTexture);
    }

    for (const EncodedFeaturesMetadata::EncodedPropertyTable& propertyTable :
         gltfComponent.EncodedMetadata.propertyTables) {
      EncodedFeaturesMetadata::SetPropertyTableParameterValues(
          pMaterial,
          association,
          index,
          propertyTable);
    }
  }
}

static void SetMetadataFeatureTableParameterValues_DEPRECATED(
    const CesiumEncodedMetadataUtility::EncodedMetadataFeatureTable&
        encodedFeatureTable,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  for (const CesiumEncodedMetadataUtility::EncodedMetadataProperty&
           encodedProperty : encodedFeatureTable.encodedProperties) {

    pMaterial->SetTextureParameterValueByInfo(
        FMaterialParameterInfo(FName(encodedProperty.name), association, index),
        encodedProperty.pTexture->pTexture->getUnrealTexture());
  }
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static void SetMetadataParameterValues_DEPRECATED(
    const CesiumGltf::Model& model,
    UCesiumGltfComponent& gltfComponent,
    LoadedPrimitiveResult& loadResult,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  /**
   * The following is the naming convention for deprecated encoded metadata:
   *
   * Feature Id Textures:
   *  - Base: "FIT_<feature table name>_"...
   *    - Texture: ..."TX"
   *    - Texture Coordinate Index: ..."UV"
   *    - Channel Mask: ..."CM"
   *
   * Feature Id Attributes:
   *  - Texture Coordinate Index (feature ids are encoded into UVs):
   *    "FA_<feature table name>"
   *
   * Feature Texture Properties:
   *  - Base: "FTX_<feature texture name>_<property name>_"...
   *    - Texture: ..."TX"
   *    - Texture Coordinate Index: ..."UV"
   *    - Swizzle: ..."SW"
   *
   * Encoded Feature Table Properties:
   *  - Encoded Property Table:
   *    "FTB_<feature table name>_<property name>"
   */

  if (!loadResult.EncodedMetadata_DEPRECATED ||
      !encodeMetadataPrimitiveGameThreadPart(
          *loadResult.EncodedMetadata_DEPRECATED)) {
    return;
  }

  for (const auto& textureCoordinateSet :
       loadResult.FeaturesMetadataTexCoordParameters) {
    pMaterial->SetScalarParameterValueByInfo(
        FMaterialParameterInfo(
            FName(textureCoordinateSet.Key),
            association,
            index),
        textureCoordinateSet.Value);
  }

  for (const FString& featureTextureName :
       loadResult.EncodedMetadata_DEPRECATED->featureTextureNames) {
    CesiumEncodedMetadataUtility::EncodedFeatureTexture*
        pEncodedFeatureTexture =
            gltfComponent.EncodedMetadata_DEPRECATED->encodedFeatureTextures
                .Find(featureTextureName);

    if (pEncodedFeatureTexture) {
      for (CesiumEncodedMetadataUtility::EncodedFeatureTextureProperty&
               encodedProperty : pEncodedFeatureTexture->properties) {

        pMaterial->SetTextureParameterValueByInfo(
            FMaterialParameterInfo(
                FName(encodedProperty.baseName + "TX"),
                association,
                index),
            encodedProperty.pTexture->pTexture->getUnrealTexture());

        pMaterial->SetVectorParameterValueByInfo(
            FMaterialParameterInfo(
                FName(encodedProperty.baseName + "SW"),
                association,
                index),
            FLinearColor(
                encodedProperty.channelOffsets[0],
                encodedProperty.channelOffsets[1],
                encodedProperty.channelOffsets[2],
                encodedProperty.channelOffsets[3]));
      }
    }
  }

  for (CesiumEncodedMetadataUtility::EncodedFeatureIdTexture&
           encodedFeatureIdTexture :
       loadResult.EncodedMetadata_DEPRECATED->encodedFeatureIdTextures) {

    pMaterial->SetTextureParameterValueByInfo(
        FMaterialParameterInfo(
            FName(encodedFeatureIdTexture.baseName + "TX"),
            association,
            index),
        encodedFeatureIdTexture.pTexture->pTexture->getUnrealTexture());

    FLinearColor channelMask;
    switch (encodedFeatureIdTexture.channel) {
    case 1:
      channelMask = FLinearColor::Green;
      break;
    case 2:
      channelMask = FLinearColor::Blue;
      break;
    default:
      channelMask = FLinearColor::Red;
    }

    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            FName(encodedFeatureIdTexture.baseName + "CM"),
            association,
            index),
        channelMask);

    const CesiumEncodedMetadataUtility::EncodedMetadataFeatureTable*
        pEncodedFeatureTable =
            gltfComponent.EncodedMetadata_DEPRECATED->encodedFeatureTables.Find(
                encodedFeatureIdTexture.featureTableName);

    if (pEncodedFeatureTable) {
      SetMetadataFeatureTableParameterValues_DEPRECATED(
          *pEncodedFeatureTable,
          pMaterial,
          association,
          index);
    }
  }

  for (const CesiumEncodedMetadataUtility::EncodedFeatureIdAttribute&
           encodedFeatureIdAttribute :
       loadResult.EncodedMetadata_DEPRECATED->encodedFeatureIdAttributes) {
    const CesiumEncodedMetadataUtility::EncodedMetadataFeatureTable*
        pEncodedFeatureTable =
            gltfComponent.EncodedMetadata_DEPRECATED->encodedFeatureTables.Find(
                encodedFeatureIdAttribute.featureTableName);

    if (pEncodedFeatureTable) {
      SetMetadataFeatureTableParameterValues_DEPRECATED(
          *pEncodedFeatureTable,
          pMaterial,
          association,
          index);
    }
  }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#pragma endregion

namespace {
void addInstanceFeatureIds(
    UCesiumGltfInstancedComponent* pInstancedComponent,
    const FCesiumFeaturesMetadataDescription& featuresDescription) {
  const TSharedPtr<FCesiumPrimitiveFeatures>& pInstanceFeatures =
      pInstancedComponent->pInstanceFeatures;
  if (!pInstanceFeatures) {
    return;
  }
  const TArray<FCesiumFeatureIdSet>& allFeatureIdSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
          *pInstanceFeatures);

  const TArray<FCesiumFeatureIdSetDescription>& featureIDSetDescriptions =
      featuresDescription.PrimitiveFeatures.FeatureIdSets;

  int32_t featureIdTextureCounter = 0;

  TArray<int32> activeFeatureIdSets;

  for (int32 i = 0; i < allFeatureIdSets.Num(); ++i) {
    FString name = EncodedFeaturesMetadata::getNameForFeatureIDSet(
        allFeatureIdSets[i],
        featureIdTextureCounter);

    const FCesiumFeatureIdSetDescription* pDescription =
        featureIDSetDescriptions.FindByPredicate(
            [&name](
                const FCesiumFeatureIdSetDescription& existingFeatureIDSet) {
              return existingFeatureIDSet.Name == name;
            });

    if (pDescription) {
      activeFeatureIdSets.Emplace(i);
    }
  }

  int32 featureSetCount = activeFeatureIdSets.Num();
  if (featureSetCount == 0) {
    return;
  }
  pInstancedComponent->SetNumCustomDataFloats(featureSetCount);
  int32 numInstances = pInstancedComponent->GetInstanceCount();
  pInstancedComponent->PerInstanceSMCustomData.SetNum(
      featureSetCount * numInstances);
  for (int32 j = 0; j < featureSetCount; ++j) {
    int64_t setIndex = activeFeatureIdSets[j];

    for (int32 i = 0; i < numInstances; ++i) {
      int64 featureId =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromInstance(
              *pInstanceFeatures,
              i,
              setIndex);
      pInstancedComponent
          ->SetCustomDataValue(i, j, static_cast<float>(featureId), true);
    }
  }
}
} // namespace

static void loadPrimitiveGameThreadPart(
    CesiumGltf::Model& model,
    UCesiumGltfComponent* pGltf,
    LoadedPrimitiveResult& loadResult,
    const glm::dmat4x4& cesiumToUnrealTransform,
    const Cesium3DTilesSelection::Tile& tile,
    bool createNavCollision,
    ACesium3DTileset* pTilesetActor,
    const std::vector<FTransform>& instanceTransforms,
    const TSharedPtr<FCesiumPrimitiveFeatures>& pInstanceFeatures) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LoadPrimitive)

#if DEBUG_GLTF_ASSET_NAMES
  FName componentName = createSafeName(loadResult.name, "");
#else
  FName componentName = "";
#endif

  const Cesium3DTilesSelection::BoundingVolume& boundingVolume =
      tile.getContentBoundingVolume().value_or(tile.getBoundingVolume());

  CesiumGltf::MeshPrimitive& meshPrimitive =
      model.meshes[loadResult.meshIndex].primitives[loadResult.primitiveIndex];

  UStaticMeshComponent* pMesh = nullptr;
  ICesiumPrimitive* pCesiumPrimitive = nullptr;
  bool createMesh = true;
  if (meshPrimitive.hasExtension<CesiumGltf::ExtensionKhrGaussianSplatting>()) {
    UCesiumGltfGaussianSplatComponent* pGaussianSplat =
        NewObject<UCesiumGltfGaussianSplatComponent>(pGltf, componentName);
    pGaussianSplat->Dimensions = loadResult.dimensions;
    // UCesiumGltfGaussianSplatComponent works differently to other primitives -
    // it just acts as a source of data for UCesiumGaussianSplatSystem to
    // accumulate and render. We do not need to create a mesh from it.
    pGaussianSplat->SetData(model, meshPrimitive);
    pGaussianSplat->SetupAttachment(pGltf);
    pGaussianSplat->RegisterComponent();
    pGaussianSplat->RegisterWithSubsystem();
    pCesiumPrimitive = pGaussianSplat;
    createMesh = false;
  } else if (meshPrimitive.mode == CesiumGltf::MeshPrimitive::Mode::POINTS) {
    UCesiumGltfPointsComponent* pPointMesh =
        NewObject<UCesiumGltfPointsComponent>(pGltf, componentName);
    pPointMesh->UsesAdditiveRefinement =
        tile.getRefine() == Cesium3DTilesSelection::TileRefine::Add;
    pPointMesh->GeometricError = static_cast<float>(tile.getGeometricError());
    pPointMesh->Dimensions = loadResult.dimensions;
    pMesh = pPointMesh;
    pCesiumPrimitive = pPointMesh;
  } else if (!instanceTransforms.empty()) {
    auto* pInstancedComponent =
        NewObject<UCesiumGltfInstancedComponent>(pGltf, componentName);
    pMesh = pInstancedComponent;
    for (const FTransform& transform : instanceTransforms) {
      pInstancedComponent->AddInstance(transform, false);
    }
    pInstancedComponent->pInstanceFeatures = pInstanceFeatures;

    const std::optional<FCesiumFeaturesMetadataDescription>&
        maybeFeaturesDescription =
            pTilesetActor->getFeaturesMetadataDescription();
    if (maybeFeaturesDescription) {
      addInstanceFeatureIds(pInstancedComponent, *maybeFeaturesDescription);
    }

    pCesiumPrimitive = pInstancedComponent;
  } else {
    auto* pComponent =
        NewObject<UCesiumGltfPrimitiveComponent>(pGltf, componentName);
    pMesh = pComponent;
    pCesiumPrimitive = pComponent;
  }

  CesiumPrimitiveData& primData = pCesiumPrimitive->getPrimitiveData();

  primData.pTilesetActor = pTilesetActor;
  primData.overlayTextureCoordinateIDToUVIndex =
      loadResult.overlayTextureCoordinateIDToUVIndex;
  primData.GltfToUnrealTexCoordMap =
      std::move(loadResult.GltfToUnrealTexCoordMap);
  primData.TexCoordAccessorMap = std::move(loadResult.TexCoordAccessorMap);
  primData.PositionAccessor = std::move(loadResult.PositionAccessor);
  primData.IndexAccessor = std::move(loadResult.IndexAccessor);
  primData.HighPrecisionNodeTransform = loadResult.transform;
  pCesiumPrimitive->UpdateTransformFromCesium(cesiumToUnrealTransform);

  if (!createMesh) {
    return;
  }

  UStaticMesh* pStaticMesh;
  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetupMesh)
    pMesh->bUseDefaultCollision = false;
    pMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    pMesh->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    primData.pModel = &model;
    primData.pMeshPrimitive = &meshPrimitive;
    primData.boundingVolume = boundingVolume;
    pMesh->SetRenderCustomDepth(pGltf->CustomDepthParameters.RenderCustomDepth);
    pMesh->SetCustomDepthStencilWriteMask(
        pGltf->CustomDepthParameters.CustomDepthStencilWriteMask);
    pMesh->SetCustomDepthStencilValue(
        pGltf->CustomDepthParameters.CustomDepthStencilValue);
    if (loadResult.isUnlit) {
      pMesh->bCastDynamicShadow = false;
    }
    pMesh->RuntimeVirtualTextures =
        primData.pTilesetActor->GetRuntimeVirtualTextures();
    pMesh->VirtualTextureRenderPassType =
        primData.pTilesetActor->GetVirtualTextureRenderPassType();
    pMesh->TranslucencySortPriority =
        primData.pTilesetActor->GetTranslucencySortPriority();

    pStaticMesh = NewObject<UStaticMesh>(pMesh, componentName);
    // Not only does the concept of ray tracing a point cloud not make much
    // sense, but if Unreal will crash trying to generate ray tracing
    // information for a static mesh without triangles.
    pStaticMesh->bSupportRayTracing =
        meshPrimitive.mode != CesiumGltf::MeshPrimitive::Mode::POINTS;
    pMesh->SetStaticMesh(pStaticMesh);

    pStaticMesh->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    pStaticMesh->NeverStream = true;

    pStaticMesh->SetRenderData(std::move(loadResult.RenderData));
  }

  const CesiumGltf::Material& material =
      loadResult.materialIndex != -1 ? model.materials[loadResult.materialIndex]
                                     : defaultMaterial;

  const CesiumGltf::MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  const FName ImportedSlotName(
      *(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));

  const auto is_in_blend_mode = [&model](auto& result) {
    return result.materialIndex != -1 &&
           model.materials[result.materialIndex].alphaMode ==
               CesiumGltf::Material::AlphaMode::BLEND;
  };

#if PLATFORM_MAC
  // TODO: figure out why water material crashes mac
  UMaterialInterface* pUserDesignatedMaterial =
      is_in_blend_mode(loadResult) ? pGltf->BaseMaterialWithTranslucency
                                   : pGltf->BaseMaterial;
#else
  UMaterialInterface* pUserDesignatedMaterial;
  if (loadResult.onlyWater || !loadResult.onlyLand) {
    pUserDesignatedMaterial = pGltf->BaseMaterialWithWater;
  } else {
    pUserDesignatedMaterial = is_in_blend_mode(loadResult)
                                  ? pGltf->BaseMaterialWithTranslucency
                                  : pGltf->BaseMaterial;
  }
#endif

  UMaterialInstanceDynamic* pMaterialForGltfPrimitive;
  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetupMaterial)

    UMaterialInstanceDynamic* pUserDesignatedMaterialAsDynamic =
        Cast<UMaterialInstanceDynamic>(pUserDesignatedMaterial);

    // If the user-designated material is a UMaterialInstanceDynamic, Create()
    // will reject it as a valid instance parent.  Defer to its non-dynamic
    // parent instead.
    UMaterialInterface* pBaseMaterial =
        pUserDesignatedMaterialAsDynamic
            ? pUserDesignatedMaterialAsDynamic->Parent.Get()
            : pUserDesignatedMaterial;

    pMaterialForGltfPrimitive = UMaterialInstanceDynamic::Create(
        pBaseMaterial,
        nullptr,
        ImportedSlotName);

    pMaterialForGltfPrimitive->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    SetGltfParameterValues(
        model,
        loadResult,
        material,
        pbr,
        pMaterialForGltfPrimitive,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);
    SetWaterParameterValues(
        model,
        loadResult,
        pMaterialForGltfPrimitive,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);

    // The base material might be a Material, or it might be a MaterialInstance.
    // Only MaterialInstances can use the material layer system, so only
    // MaterialInstances will have UCesiumMaterialUserData.
    UMaterialInstance* pBaseAsMaterialInstance =
        Cast<UMaterialInstance>(pBaseMaterial);

    UCesiumMaterialUserData* pCesiumData =
        pBaseAsMaterialInstance
            ? pBaseAsMaterialInstance
                  ->GetAssetUserData<UCesiumMaterialUserData>()
            : nullptr;

    // If possible and necessary, attach the CesiumMaterialUserData now.
#if WITH_EDITORONLY_DATA
    if (pBaseAsMaterialInstance && !pCesiumData) {
      const FStaticParameterSet& parameters =
          pBaseAsMaterialInstance->GetStaticParameters();

      bool hasLayers = parameters.bHasMaterialLayers;
      if (hasLayers) {
#if WITH_EDITOR
        FScopedTransaction transaction(
            FText::FromString("Add Cesium User Data to Material"));
        pBaseAsMaterialInstance->Modify();
#endif
        pCesiumData = NewObject<UCesiumMaterialUserData>(
            pBaseAsMaterialInstance,
            NAME_None,
            RF_Transactional);
        pBaseAsMaterialInstance->AddAssetUserData(pCesiumData);
        pCesiumData->PostEditChangeOwner();
      }
    }
#endif

    // If CesiumMaterialUserData was not attached (e.g., material was
    // dynamically created at runtime), then walk the parent chain of the
    // material to find it.
    while (pBaseAsMaterialInstance && !pCesiumData) {
      pBaseAsMaterialInstance =
          Cast<UMaterialInstance>(pBaseAsMaterialInstance->Parent.Get());
      if (pBaseAsMaterialInstance) {
        pCesiumData = pBaseAsMaterialInstance
                          ->GetAssetUserData<UCesiumMaterialUserData>();
      }
    }

    if (pCesiumData) {
      SetGltfParameterValues(
          model,
          loadResult,
          material,
          pbr,
          pMaterialForGltfPrimitive,
          EMaterialParameterAssociation::LayerParameter,
          0);

      // Initialize fade uniform to fully visible, in case LOD transitions
      // are off.
      int fadeLayerIndex = pCesiumData->LayerNames.Find("DitherFade");
      if (fadeLayerIndex >= 0) {
        pMaterialForGltfPrimitive->SetScalarParameterValueByInfo(
            FMaterialParameterInfo(
                "FadePercentage",
                EMaterialParameterAssociation::LayerParameter,
                fadeLayerIndex),
            1.0f);
        pMaterialForGltfPrimitive->SetScalarParameterValueByInfo(
            FMaterialParameterInfo(
                "FadingType",
                EMaterialParameterAssociation::LayerParameter,
                fadeLayerIndex),
            0.0f);
      }

      // If there's a "Water" layer, set its parameters
      int32 waterIndex = pCesiumData->LayerNames.Find("Water");
      if (waterIndex >= 0) {
        SetWaterParameterValues(
            model,
            loadResult,
            pMaterialForGltfPrimitive,
            EMaterialParameterAssociation::LayerParameter,
            waterIndex);
      }

      int32 featuresMetadataIndex =
          pCesiumData->LayerNames.Find("FeaturesMetadata");
      int32 metadataIndex = pCesiumData->LayerNames.Find("Metadata");
      if (featuresMetadataIndex >= 0) {
        SetFeaturesMetadataParameterValues(
            model,
            *pGltf,
            loadResult,
            pMaterialForGltfPrimitive,
            EMaterialParameterAssociation::LayerParameter,
            featuresMetadataIndex);
      } else if (metadataIndex >= 0) {
        // Set parameters for materials generated by the old implementation
        SetMetadataParameterValues_DEPRECATED(
            model,
            *pGltf,
            loadResult,
            pMaterialForGltfPrimitive,
            EMaterialParameterAssociation::LayerParameter,
            metadataIndex);
      }
    }

    if (pUserDesignatedMaterialAsDynamic) {
      // Ensure any parameters on the original UMaterialInstanceDynamic are
      // transferred to the copy.
      for (auto& it : pUserDesignatedMaterialAsDynamic->ScalarParameterValues) {
        pMaterialForGltfPrimitive->SetScalarParameterValueByInfo(
            it.ParameterInfo,
            it.ParameterValue);
      }

      for (auto& it : pUserDesignatedMaterialAsDynamic->VectorParameterValues) {
        pMaterialForGltfPrimitive->SetVectorParameterValueByInfo(
            it.ParameterInfo,
            it.ParameterValue);
      }

      for (auto& it :
           pUserDesignatedMaterialAsDynamic->DoubleVectorParameterValues) {
        pMaterialForGltfPrimitive->SetVectorParameterValueByInfo(
            it.ParameterInfo,
            it.ParameterValue);
      }

      for (auto& it :
           pUserDesignatedMaterialAsDynamic->TextureParameterValues) {
        pMaterialForGltfPrimitive->SetTextureParameterValueByInfo(
            it.ParameterInfo,
            it.ParameterValue);
      }

      for (auto& it : pUserDesignatedMaterialAsDynamic->FontParameterValues) {
        pMaterialForGltfPrimitive->SetFontParameterValue(
            it.ParameterInfo,
            it.FontValue,
            it.FontPage);
      }
    }
  }

  primData.Features = std::move(loadResult.Features);
  primData.Metadata = std::move(loadResult.Metadata);

  primData.EncodedFeatures = std::move(loadResult.EncodedFeatures);
  primData.EncodedMetadata = std::move(loadResult.EncodedMetadata);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS

  // Doing the above std::move operations invalidates the pointers in the
  // FCesiumMetadataPrimitive constructed on the loadResult. It's a bit
  // awkward, but we have to reconstruct the metadata primitive here.
  primData.Metadata_DEPRECATED = FCesiumMetadataPrimitive{
      primData.Features,
      primData.Metadata,
      pGltf->Metadata};

  if (loadResult.EncodedMetadata_DEPRECATED) {
    primData.EncodedMetadata_DEPRECATED =
        std::move(loadResult.EncodedMetadata_DEPRECATED);
  }

  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  pMaterialForGltfPrimitive->TwoSided = true;

  pStaticMesh->AddMaterial(pMaterialForGltfPrimitive);

  pStaticMesh->SetLightingGuid();

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::InitResources)
    pStaticMesh->InitResources();
  }

  // Set up RenderData bounds and LOD data
  pStaticMesh->CalculateExtendedBounds();
  pStaticMesh->GetRenderData()->ScreenSize[0].Default = 1.0f;

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::BodySetup)

    pStaticMesh->CreateBodySetup();

    UBodySetup* pBodySetup = pMesh->GetBodySetup();

    // pMesh->UpdateCollisionFromStaticMesh();
    pBodySetup->CollisionTraceFlag =
        ECollisionTraceFlag::CTF_UseComplexAsSimple;

    if (loadResult.pCollisionMesh) {
      pBodySetup->TriMeshGeometries.Add(loadResult.pCollisionMesh);
    }

    // Mark physics meshes created, no matter if we actually have a collision
    // mesh or not. We don't want the editor creating collision meshes itself
    // in the game thread, because that would be slow.
    pBodySetup->bCreatedPhysicsMeshes = true;
    pBodySetup->bSupportUVsAndFaceRemap =
        UPhysicsSettings::Get()->bSupportUVFromHitResults;
  }

  if (createNavCollision) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CreateNavCollision)
    pStaticMesh->CreateNavCollision(true);
  }

  pMesh->SetMobility(pGltf->Mobility);

  pMesh->SetupAttachment(pGltf);

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::RegisterComponent)
    pMesh->RegisterComponent();
  }
}

/*static*/ CesiumAsync::Future<UCesiumGltfComponent::CreateOffGameThreadResult>
UCesiumGltfComponent::CreateOffGameThread(
    const CesiumAsync::AsyncSystem& AsyncSystem,
    const glm::dmat4x4& Transform,
    CreateModelOptions&& Options,
    const CesiumGeospatial::Ellipsoid& Ellipsoid) {
  return loadModelAnyThreadPart(
      AsyncSystem,
      Transform,
      std::move(Options),
      Ellipsoid);
}

/*static*/ UCesiumGltfComponent* UCesiumGltfComponent::CreateOnGameThread(
    CesiumGltf::Model& model,
    ACesium3DTileset* pTilesetActor,
    TUniquePtr<HalfConstructed> pHalfConstructed,
    const glm::dmat4x4& cesiumToUnrealTransform,
    UMaterialInterface* pBaseMaterial,
    UMaterialInterface* pBaseTranslucentMaterial,
    UMaterialInterface* pBaseWaterMaterial,
    FCustomDepthParameters CustomDepthParameters,
    const Cesium3DTilesSelection::Tile& tile,
    bool createNavCollision) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LoadModel)

  HalfConstructedReal* pReal =
      static_cast<HalfConstructedReal*>(pHalfConstructed.Get());

  // TODO: was this a common case before?
  // (This code checked if there were no loaded primitives in the model)
  // if (result.size() == 0) {
  //   return nullptr;
  // }

  UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pTilesetActor);
  Gltf->SetMobility(pTilesetActor->GetRootComponent()->Mobility);
  Gltf->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  Gltf->Metadata = std::move(pReal->loadModelResult.Metadata);
  Gltf->EncodedMetadata = std::move(pReal->loadModelResult.EncodedMetadata);
  Gltf->EncodedMetadata_DEPRECATED =
      std::move(pReal->loadModelResult.EncodedMetadata_DEPRECATED);

  if (pBaseMaterial) {
    Gltf->BaseMaterial = pBaseMaterial;
  }

  if (pBaseTranslucentMaterial) {
    Gltf->BaseMaterialWithTranslucency = pBaseTranslucentMaterial;
  }

  if (pBaseWaterMaterial) {
    Gltf->BaseMaterialWithWater = pBaseWaterMaterial;
  }

  Gltf->CustomDepthParameters = CustomDepthParameters;

  encodeModelMetadataGameThreadPart(Gltf->EncodedMetadata);

  if (Gltf->EncodedMetadata_DEPRECATED) {
    encodeMetadataGameThreadPart(*Gltf->EncodedMetadata_DEPRECATED);
  }

  for (LoadedNodeResult& node : pReal->loadModelResult.nodeResults) {
    if (node.meshResult) {
      for (LoadedPrimitiveResult& primitive :
           node.meshResult->primitiveResults) {
        loadPrimitiveGameThreadPart(
            model,
            Gltf,
            primitive,
            cesiumToUnrealTransform,
            tile,
            createNavCollision,
            pTilesetActor,
            node.InstanceTransforms,
            node.pInstanceFeatures);
      }
    }
  }

  Gltf->SetVisibility(false, true);
  Gltf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  return Gltf;
}

UCesiumGltfComponent::UCesiumGltfComponent() : USceneComponent() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UMaterialInstance> BaseMaterial;
    ConstructorHelpers::FObjectFinder<UMaterialInstance>
        BaseMaterialWithTranslucency;
    ConstructorHelpers::FObjectFinder<UMaterialInstance> BaseMaterialWithWater;
    ConstructorHelpers::FObjectFinder<UTexture2D> Transparent1x1;
    FConstructorStatics()
        : BaseMaterial(TEXT(
              "/CesiumForUnreal/Materials/Instances/MI_CesiumThreeOverlaysAndClipping.MI_CesiumThreeOverlaysAndClipping")),
          BaseMaterialWithTranslucency(TEXT(
              "/CesiumForUnreal/Materials/Instances/MI_CesiumThreeOverlaysAndClippingTranslucent.MI_CesiumThreeOverlaysAndClippingTranslucent")),
          BaseMaterialWithWater(TEXT(
              "/CesiumForUnreal/Materials/Instances/MI_CesiumThreeOverlaysAndClippingAndWater.MI_CesiumThreeOverlaysAndClippingAndWater")),
          Transparent1x1(
              TEXT("/CesiumForUnreal/Textures/transparent1x1.transparent1x1")) {
    }
  };
  static FConstructorStatics ConstructorStatics;

  this->BaseMaterial = ConstructorStatics.BaseMaterial.Object;
  this->BaseMaterialWithTranslucency =
      ConstructorStatics.BaseMaterialWithTranslucency.Object;
  this->BaseMaterialWithWater = ConstructorStatics.BaseMaterialWithWater.Object;
  this->Transparent1x1 = ConstructorStatics.Transparent1x1.Object;

  PrimaryComponentTick.bCanEverTick = false;
}

void UCesiumGltfComponent::UpdateTransformFromCesium(
    const glm::dmat4& cesiumToUnrealTransform) {
  for (USceneComponent* pSceneComponent : this->GetAttachChildren()) {
    if (auto* pCesiumPrimitive = Cast<ICesiumPrimitive>(pSceneComponent)) {
      pCesiumPrimitive->UpdateTransformFromCesium(cesiumToUnrealTransform);
    }
  }
}

namespace {
template <typename Func>
void forEachPrimitiveComponent(UCesiumGltfComponent* pGltf, Func&& f) {
  for (USceneComponent* pSceneComponent : pGltf->GetAttachChildren()) {
    UCesiumGltfPrimitiveComponent* pPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pSceneComponent);
    if (pPrimitive) {
      UMaterialInstanceDynamic* pMaterial =
          Cast<UMaterialInstanceDynamic>(pPrimitive->GetMaterial(0));

      if (!IsValid(pMaterial) || pMaterial->IsUnreachable()) {
        // Don't try to update the material while it's in the process of
        // being destroyed. This can lead to the render thread freaking out
        // when it's asked to update a parameter for a material that has
        // been marked for garbage collection.
        continue;
      }

      UMaterialInterface* pBaseMaterial = pMaterial->Parent;
      UMaterialInstance* pBaseAsMaterialInstance =
          Cast<UMaterialInstance>(pBaseMaterial);
      UCesiumMaterialUserData* pCesiumData =
          pBaseAsMaterialInstance
              ? pBaseAsMaterialInstance
                    ->GetAssetUserData<UCesiumMaterialUserData>()
              : nullptr;

      f(pPrimitive, pMaterial, pCesiumData);
    }
  }
} // namespace

} // namespace

void UCesiumGltfComponent::AttachRasterTile(
    const Cesium3DTilesSelection::Tile& tile,
    const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
    UTexture2D* pTexture,
    const glm::dvec2& translation,
    const glm::dvec2& scale,
    int32 textureCoordinateID) {
  FVector4 translationAndScale(translation.x, translation.y, scale.x, scale.y);

  forEachPrimitiveComponent(
      this,
      [&rasterTile, pTexture, &translationAndScale, textureCoordinateID](
          UCesiumGltfPrimitiveComponent* pPrimitive,
          UMaterialInstanceDynamic* pMaterial,
          UCesiumMaterialUserData* pCesiumData) {
        CesiumPrimitiveData& primData = pPrimitive->getPrimitiveData();
        // If this material uses material layers and has the Cesium user data,
        // set the parameters on each material layer that maps to this overlay
        // tile.
        if (pCesiumData) {
          FString name(
              UTF8_TO_TCHAR(rasterTile.getOverlay().getName().c_str()));

          for (int32 i = 0; i < pCesiumData->LayerNames.Num(); ++i) {
            if (pCesiumData->LayerNames[i] != name) {
              continue;
            }

            pMaterial->SetTextureParameterValueByInfo(
                FMaterialParameterInfo(
                    "Texture",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                pTexture);
            pMaterial->SetVectorParameterValueByInfo(
                FMaterialParameterInfo(
                    "TranslationScale",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                translationAndScale);
            check(
                textureCoordinateID >= 0 &&
                textureCoordinateID <
                    primData.overlayTextureCoordinateIDToUVIndex.size());
            pMaterial->SetScalarParameterValueByInfo(
                FMaterialParameterInfo(
                    "TextureCoordinateIndex",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                static_cast<float>(primData.overlayTextureCoordinateIDToUVIndex
                                       [textureCoordinateID]));
          }
        } else {
          pMaterial->SetTextureParameterValue(
              createSafeName(rasterTile.getOverlay().getName(), "_Texture"),
              pTexture);
          pMaterial->SetVectorParameterValue(
              createSafeName(
                  rasterTile.getOverlay().getName(),
                  "_TranslationScale"),
              translationAndScale);
          pMaterial->SetScalarParameterValue(
              createSafeName(
                  rasterTile.getOverlay().getName(),
                  "_TextureCoordinateIndex"),
              static_cast<float>(primData.overlayTextureCoordinateIDToUVIndex
                                     [textureCoordinateID]));
        }
      });
}

void UCesiumGltfComponent::DetachRasterTile(
    const Cesium3DTilesSelection::Tile& tile,
    const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
    UTexture2D* pTexture) {
  forEachPrimitiveComponent(
      this,
      [this, &rasterTile, pTexture](
          UCesiumGltfPrimitiveComponent* pPrimitive,
          UMaterialInstanceDynamic* pMaterial,
          UCesiumMaterialUserData* pCesiumData) {
        // If this material uses material layers and has the Cesium user data,
        // clear the parameters on each material layer that maps to this
        // overlay tile.
        if (pCesiumData) {
          FString name(
              UTF8_TO_TCHAR(rasterTile.getOverlay().getName().c_str()));
          for (int32 i = 0; i < pCesiumData->LayerNames.Num(); ++i) {
            if (pCesiumData->LayerNames[i] != name) {
              continue;
            }

            pMaterial->SetTextureParameterValueByInfo(
                FMaterialParameterInfo(
                    "Texture",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                this->Transparent1x1);
          }
        } else {
          pMaterial->SetTextureParameterValue(
              createSafeName(rasterTile.getOverlay().getName(), "_Texture"),
              this->Transparent1x1);
        }
      });
}

void UCesiumGltfComponent::SetCollisionEnabled(
    ECollisionEnabled::Type NewType) {
  for (USceneComponent* pSceneComponent : this->GetAttachChildren()) {
    UCesiumGltfPrimitiveComponent* pPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pSceneComponent);
    if (pPrimitive) {
      pPrimitive->SetCollisionEnabled(NewType);
    }
  }
}

void UCesiumGltfComponent::BeginDestroy() {
  // Clear everything we can in order to reduce memory usage, because this
  // UObject might not actually get deleted by the garbage collector until
  // much later.
  this->Metadata = FCesiumModelMetadata();
  this->EncodedMetadata = EncodedFeaturesMetadata::EncodedModelMetadata();

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  this->EncodedMetadata_DEPRECATED.reset();
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  Super::BeginDestroy();
}

void UCesiumGltfComponent::UpdateFade(float fadePercentage, bool fadingIn) {
  if (!this->IsVisible()) {
    return;
  }

  UCesiumMaterialUserData* pCesiumData =
      BaseMaterial->GetAssetUserData<UCesiumMaterialUserData>();

  if (!pCesiumData) {
    return;
  }

  int fadeLayerIndex = pCesiumData->LayerNames.Find("DitherFade");
  if (fadeLayerIndex < 0) {
    return;
  }

  fadePercentage = glm::clamp(fadePercentage, 0.0f, 1.0f);

  for (USceneComponent* pChild : this->GetAttachChildren()) {
    UCesiumGltfPrimitiveComponent* pPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pChild);
    if (!pPrimitive || pPrimitive->GetMaterials().IsEmpty()) {
      continue;
    }

    UMaterialInstanceDynamic* pMaterial =
        Cast<UMaterialInstanceDynamic>(pPrimitive->GetMaterials()[0]);
    if (!pMaterial) {
      continue;
    }

    pMaterial->SetScalarParameterValueByInfo(
        FMaterialParameterInfo(
            "FadePercentage",
            EMaterialParameterAssociation::LayerParameter,
            fadeLayerIndex),
        fadePercentage);
    pMaterial->SetScalarParameterValueByInfo(
        FMaterialParameterInfo(
            "FadingType",
            EMaterialParameterAssociation::LayerParameter,
            fadeLayerIndex),
        fadingIn ? 0.0f : 1.0f);
  }
}

template <typename TIndex>
static Chaos::FTriangleMeshImplicitObjectPtr BuildChaosTriangleMeshes(
    const FPositionVertexBuffer& positionBuffer,
    const TArray<uint32>& indices) {
  uint32 vertexCount = positionBuffer.GetNumVertices();

  Chaos::TParticles<Chaos::FRealSingle, 3> vertices;
  vertices.AddParticles(vertexCount);
  for (uint32 i = 0; i < vertexCount; ++i) {
    vertices.X(int32(i)) = positionBuffer.VertexPosition(i);
  }

  int32 triangleCount = indices.Num() / 3;
  TArray<Chaos::TVector<TIndex, 3>> triangles;
  TArray<int32> faceRemap;

  triangles.Reserve(triangleCount);
  faceRemap.Reserve(triangleCount);

  for (int32 i = 0; i < triangleCount; ++i) {
    const int32 index0 = 3 * i;
    int32 vIndex0 = int32(indices[index0 + 1]);
    int32 vIndex1 = int32(indices[index0]);
    int32 vIndex2 = int32(indices[index0 + 2]);

    triangles.Add(Chaos::TVector<int32, 3>(vIndex0, vIndex1, vIndex2));
    faceRemap.Add(i);
  }

  TUniquePtr<TArray<int32>> pFaceRemap = MakeUnique<TArray<int32>>(faceRemap);
  TArray<uint16> materials;
  materials.SetNum(triangles.Num());

  return new Chaos::FTriangleMeshImplicitObject(
      MoveTemp(vertices),
      MoveTemp(triangles),
      MoveTemp(materials),
      MoveTemp(pFaceRemap),
      nullptr,
      false);
}
