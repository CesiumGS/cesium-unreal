// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfComponent.h"
#include "Async/Async.h"
#include "CesiumCommon.h"
#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumFeatureIdSet.h"
#include "CesiumGltfPointsComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMaterialUserData.h"
#include "CesiumRasterOverlays.h"
#include "CesiumRuntime.h"
#include "CesiumTextureUtility.h"
#include "CesiumTransforms.h"
#include "Chaos/AABBTree.h"
#include "Chaos/CollisionConvexMesh.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "CreateGltfOptions.h"
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
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionKhrMaterialsUnlit.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/TextureInfo.h>
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

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

using namespace CesiumGltf;
using namespace CesiumTextureUtility;
using namespace CreateGltfOptions;
using namespace LoadGltfResult;

namespace {
using TMeshVector2 = FVector2f;
using TMeshVector3 = FVector3f;
using TMeshVector4 = FVector4f;
} // namespace

static uint32_t nextMaterialId = 0;

namespace {
class HalfConstructedReal : public UCesiumGltfComponent::HalfConstructed {
public:
  LoadModelResult loadModelResult{};
};
} // namespace

template <class... T> struct IsAccessorView;

template <class T> struct IsAccessorView<T> : std::false_type {};

template <class T> struct IsAccessorView<AccessorView<T>> : std::true_type {};

template <class T>
static uint32_t updateTextureCoordinates(
    const Model& model,
    const MeshPrimitive& primitive,
    bool duplicateVertices,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices,
    const std::optional<T>& texture,
    std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap) {
  if (!texture) {
    return 0;
  }

  return updateTextureCoordinates(
      model,
      primitive,
      duplicateVertices,
      vertices,
      indices,
      "TEXCOORD_" + std::to_string(texture.value().texCoord),
      gltfToUnrealTexCoordMap);
}

uint32_t updateTextureCoordinates(
    const Model& model,
    const MeshPrimitive& primitive,
    bool duplicateVertices,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices,
    const std::string& attributeName,
    std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap) {
  auto uvAccessorIt = primitive.attributes.find(attributeName);
  if (uvAccessorIt == primitive.attributes.end()) {
    // Texture not used, texture coordinates don't matter.
    return 0;
  }

  int32_t uvAccessorID = uvAccessorIt->second;
  auto mapIt = gltfToUnrealTexCoordMap.find(uvAccessorID);
  if (mapIt != gltfToUnrealTexCoordMap.end()) {
    // Texture coordinates for this accessor are already populated.
    return mapIt->second;
  }

  size_t textureCoordinateIndex = gltfToUnrealTexCoordMap.size();
  gltfToUnrealTexCoordMap[uvAccessorID] = textureCoordinateIndex;

  AccessorView<TMeshVector2> uvAccessor(model, uvAccessorID);
  if (uvAccessor.status() != AccessorViewStatus::Valid) {
    return 0;
  }

  if (duplicateVertices) {
    for (int i = 0; i < indices.Num(); ++i) {
      FStaticMeshBuildVertex& vertex = vertices[i];
      uint32 vertexIndex = indices[i];
      if (vertexIndex >= 0 && vertexIndex < uvAccessor.size()) {
        vertex.UVs[textureCoordinateIndex] = uvAccessor[vertexIndex];
      } else {
        vertex.UVs[textureCoordinateIndex] = TMeshVector2(0.0f, 0.0f);
      }
    }
  } else {
    for (int i = 0; i < vertices.Num(); ++i) {
      FStaticMeshBuildVertex& vertex = vertices[i];
      if (i >= 0 && i < uvAccessor.size()) {
        vertex.UVs[textureCoordinateIndex] = uvAccessor[i];
      } else {
        vertex.UVs[textureCoordinateIndex] = TMeshVector2(0.0f, 0.0f);
      }
    }
  }

  return textureCoordinateIndex;
}

static int mikkGetNumFaces(const SMikkTSpaceContext* Context) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  return vertices.Num() / 3;
}

static int
mikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  return FaceIdx < (vertices.Num() / 3) ? 3 : 0;
}

static void mikkGetPosition(
    const SMikkTSpaceContext* Context,
    float Position[3],
    const int FaceIdx,
    const int VertIdx) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  const TMeshVector3& position = vertices[FaceIdx * 3 + VertIdx].Position;
  Position[0] = position.X;
  Position[1] = -position.Y;
  Position[2] = position.Z;
}

static void mikkGetNormal(
    const SMikkTSpaceContext* Context,
    float Normal[3],
    const int FaceIdx,
    const int VertIdx) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  const TMeshVector3& normal = vertices[FaceIdx * 3 + VertIdx].TangentZ;
  Normal[0] = normal.X;
  Normal[1] = -normal.Y;
  Normal[2] = normal.Z;
}

static void mikkGetTexCoord(
    const SMikkTSpaceContext* Context,
    float UV[2],
    const int FaceIdx,
    const int VertIdx) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  const TMeshVector2& uv = vertices[FaceIdx * 3 + VertIdx].UVs[0];
  UV[0] = uv.X;
  UV[1] = uv.Y;
}

static void mikkSetTSpaceBasic(
    const SMikkTSpaceContext* Context,
    const float Tangent[3],
    const float BitangentSign,
    const int FaceIdx,
    const int VertIdx) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  FStaticMeshBuildVertex& vertex = vertices[FaceIdx * 3 + VertIdx];

  FVector3f TangentZ = vertex.TangentZ;
  TangentZ.Y = -TangentZ.Y;

  FVector3f TangentX = TMeshVector3(Tangent[0], Tangent[1], Tangent[2]);
  FVector3f TangentY =
      BitangentSign * TMeshVector3::CrossProduct(TangentZ, TangentX);

  TangentX.Y = -TangentX.Y;
  TangentY.Y = -TangentY.Y;

  vertex.TangentX = TangentX;
  vertex.TangentY = TangentY;
}

static void computeTangentSpace(TArray<FStaticMeshBuildVertex>& vertices) {
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

static void setUniformNormals(
    TArray<FStaticMeshBuildVertex>& vertices,
    TMeshVector3 normal) {
  for (int i = 0; i < vertices.Num(); i++) {
    FStaticMeshBuildVertex& v = vertices[i];
    v.TangentX = v.TangentY = TMeshVector3(0.0f);
    v.TangentZ = normal;
  }
}

static void computeFlatNormals(TArray<FStaticMeshBuildVertex>& vertices) {
  // Compute flat normals
  for (int i = 0; i < vertices.Num(); i += 3) {
    FStaticMeshBuildVertex& v0 = vertices[i];
    FStaticMeshBuildVertex& v1 = vertices[i + 1];
    FStaticMeshBuildVertex& v2 = vertices[i + 2];

    // The Y axis has previously been inverted, so undo that before
    // computing the normal direction. Then invert the Y coordinate of the
    // normal, too.

    TMeshVector3 v01 = v1.Position - v0.Position;
    v01.Y = -v01.Y;
    TMeshVector3 v02 = v2.Position - v0.Position;
    v02.Y = -v02.Y;
    TMeshVector3 normal = TMeshVector3::CrossProduct(v01, v02);

    normal.Y = -normal.Y;

    v0.TangentX = v1.TangentX = v2.TangentX = TMeshVector3(0.0f);
    v0.TangentY = v1.TangentY = v2.TangentY = TMeshVector3(0.0f);
    v0.TangentZ = v1.TangentZ = v2.TangentZ = normal.GetSafeNormal();
  }
}

template <typename TIndex>
#if ENGINE_VERSION_5_4_OR_HIGHER
static Chaos::FTriangleMeshImplicitObjectPtr
#else
static TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
#endif
BuildChaosTriangleMeshes(
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices);

static const Material defaultMaterial;
static const MaterialPBRMetallicRoughness defaultPbrMetallicRoughness;

struct ColorVisitor {
  bool duplicateVertices;
  TArray<FStaticMeshBuildVertex>& StaticMeshBuildVertices;
  const TArray<uint32>& indices;

  bool operator()(AccessorView<nullptr_t>&& invalidView) { return false; }

  template <typename TColorView> bool operator()(TColorView&& colorView) {
    if (colorView.status() != AccessorViewStatus::Valid) {
      return false;
    }

    bool success = true;
    if (duplicateVertices) {
      for (int i = 0; success && i < this->indices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = this->StaticMeshBuildVertices[i];
        uint32 vertexIndex = this->indices[i];
        if (vertexIndex >= colorView.size()) {
          success = false;
        } else {
          success =
              ColorVisitor::convertColor(colorView[vertexIndex], vertex.Color);
        }
      }
    } else {
      for (int i = 0; success && i < this->StaticMeshBuildVertices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = this->StaticMeshBuildVertices[i];
        if (i >= colorView.size()) {
          success = false;
        } else {
          success = ColorVisitor::convertColor(colorView[i], vertex.Color);
        }
      }
    }

    return success;
  }

  template <typename TElement>
  static bool
  convertColor(const AccessorTypes::VEC3<TElement>& color, FColor& out) {
    out.A = 255;
    return convertElement(color.value[0], out.R) &&
           convertElement(color.value[1], out.G) &&
           convertElement(color.value[2], out.B);
  }

  template <typename TElement>
  static bool
  convertColor(const AccessorTypes::VEC4<TElement>& color, FColor& out) {
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
    const std::optional<T>& gltfTexture,
    bool sRGB,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {
  if (!gltfTexture || gltfTexture.value().index < 0 ||
      gltfTexture.value().index >= model.textures.size()) {
    if (gltfTexture && gltfTexture.value().index >= 0) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Texture index must be less than %d, but is %d"),
          model.textures.size(),
          gltfTexture.value().index);
    }
    return nullptr;
  }

  int32_t textureIndex = gltfTexture.value().index;
  const CesiumGltf::Texture& texture = model.textures[textureIndex];
  return loadTextureFromModelAnyThreadPart(
      model,
      texture,
      sRGB,
      textureResources);
}

static void applyWaterMask(
    Model& model,
    const MeshPrimitive& primitive,
    LoadPrimitiveResult& primitiveResult,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {
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
        TextureInfo waterMaskInfo;
        waterMaskInfo.index = waterMaskTextureId;
        if (waterMaskTextureId >= 0 &&
            waterMaskTextureId < model.textures.size()) {
          primitiveResult.waterMaskTexture = loadTexture(
              model,
              std::make_optional(waterMaskInfo),
              false,
              textureResources);
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
    const std::optional<TextureInfo>& maybeBaseColorTexture =
        material.pbrMetallicRoughness->baseColorTexture;
    if (maybeBaseColorTexture && textureUsesSpecifiedImage(
                                     model,
                                     maybeBaseColorTexture->index,
                                     imageIndex)) {
      return true;
    }

    const std::optional<TextureInfo>& maybeMetallicRoughnessTexture =
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
 * Creates texture coordinate accessors for the feature ID sets and metadata in
 * the primitive. This enables feature ID texture / property texture picking
 * without requiring UVs in the physics bodies.
 */
static void createTexCoordAccessorsForFeaturesMetadata(
    const Model& model,
    const MeshPrimitive& primitive,
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
  }
}

/**
 * Updates the primitive's information for the texture coordinates required for
 * features and metadata styling. This processes existing texture coordinate
 * sets for feature ID textures and property textures, and generates new texture
 * coordinates for attribute and implicit feature ID sets.
 */
static void updateTextureCoordinatesForFeaturesMetadata(
    const Model& model,
    const MeshPrimitive& primitive,
    bool duplicateVertices,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices,
    const FCesiumPrimitiveFeatures& primitiveFeatures,
    const CesiumEncodedFeaturesMetadata::EncodedPrimitiveFeatures&
        encodedPrimitiveFeatures,
    const CesiumEncodedFeaturesMetadata::EncodedPrimitiveMetadata&
        encodedPrimitiveMetadata,
    const CesiumEncodedFeaturesMetadata::EncodedModelMetadata&
        encodedModelMetadata,
    TMap<FString, uint32_t>& featuresMetadataTexcoordParameters,
    std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap) {

  TRACE_CPUPROFILER_EVENT_SCOPE(
      Cesium::UpdateTextureCoordinatesForFeaturesMetadata)

  for (const int64 propertyTextureIndex :
       encodedPrimitiveMetadata.propertyTextureIndices) {
    // Property textures can be made accessible in Unreal materials without
    // requiring a texture coordinate set on the primitive. If it is not present
    // in primitive metadata, then do not set the parameter.
    const CesiumEncodedFeaturesMetadata::EncodedPropertyTexture&
        encodedPropertyTexture =
            encodedModelMetadata.propertyTextures[propertyTextureIndex];

    for (const CesiumEncodedFeaturesMetadata::EncodedPropertyTextureProperty&
             encodedProperty : encodedPropertyTexture.properties) {

      FString fullPropertyName = CesiumEncodedFeaturesMetadata::
          getMaterialNameForPropertyTextureProperty(
              encodedPropertyTexture.name,
              encodedProperty.name);

      featuresMetadataTexcoordParameters.Emplace(
          fullPropertyName +
              CesiumEncodedFeaturesMetadata::MaterialTexCoordIndexSuffix,
          updateTextureCoordinates(
              model,
              primitive,
              duplicateVertices,
              vertices,
              indices,
              "TEXCOORD_" +
                  std::to_string(encodedProperty.textureCoordinateSetIndex),
              gltfToUnrealTexCoordMap));
    }
  }

  // These are necessary for retrieving feature ID attributes, since we'll be
  // taking feature IDs from the attribute itself and putting them into
  // texcoords. We could technically just make an AccessorView on the attribute,
  // but there are multiple feature ID component types, and
  // FCesiumFeatureIdAttribute already creates the accessor view for us.
  const TArray<FCesiumFeatureIdSet>& featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
          primitiveFeatures);

  for (const CesiumEncodedFeaturesMetadata::EncodedFeatureIdSet&
           encodedFeatureIDSet : encodedPrimitiveFeatures.featureIdSets) {
    FString SafeName = CesiumEncodedFeaturesMetadata::createHlslSafeName(
        encodedFeatureIDSet.name);
    if (encodedFeatureIDSet.attribute) {
      int32_t attribute = *encodedFeatureIDSet.attribute;
      std::string attributeName = "_FEATURE_ID_" + std::to_string(attribute);
      if (primitive.attributes.find(attributeName) ==
          primitive.attributes.end()) {
        continue;
      }

      // This was already validated when creating the EncodedFeatureIdSet.
      int32_t accessor = primitive.attributes.at(attributeName);

      uint32_t textureCoordinateIndex = gltfToUnrealTexCoordMap.size();
      gltfToUnrealTexCoordMap[accessor] = textureCoordinateIndex;
      featuresMetadataTexcoordParameters.Emplace(
          SafeName,
          textureCoordinateIndex);

      const FCesiumFeatureIdSet& featureIDSet =
          featureIDSets[encodedFeatureIDSet.index];
      const FCesiumFeatureIdAttribute& featureIDAttribute =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
              featureIDSet);

      int64 vertexCount =
          UCesiumFeatureIdAttributeBlueprintLibrary::GetVertexCount(
              featureIDAttribute);

      // We encode unsigned integer feature ids as floats in the u-channel of
      // a texture coordinate slot.
      if (duplicateVertices) {
        for (int64_t i = 0; i < indices.Num(); ++i) {
          FStaticMeshBuildVertex& vertex = vertices[i];
          uint32 vertexIndex = indices[i];
          if (vertexIndex >= 0 && vertexIndex < vertexCount) {
            float featureId = static_cast<float>(
                UCesiumFeatureIdAttributeBlueprintLibrary::
                    GetFeatureIDForVertex(featureIDAttribute, vertexIndex));
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(featureId, 0.0f);
          } else {
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(0.0f, 0.0f);
          }
        }
      } else {
        for (int64_t i = 0; i < vertices.Num(); ++i) {
          FStaticMeshBuildVertex& vertex = vertices[i];
          if (i < vertexCount) {
            float featureId = static_cast<float>(
                UCesiumFeatureIdAttributeBlueprintLibrary::
                    GetFeatureIDForVertex(featureIDAttribute, i));
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(featureId, 0.0f);
          } else {
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(0.0f, 0.0f);
          }
        }
      }
    } else if (encodedFeatureIDSet.texture) {
      const CesiumEncodedFeaturesMetadata::EncodedFeatureIdTexture&
          encodedFeatureIDTexture = *encodedFeatureIDSet.texture;
      featuresMetadataTexcoordParameters.Emplace(
          SafeName + CesiumEncodedFeaturesMetadata::MaterialTexCoordIndexSuffix,
          updateTextureCoordinates(
              model,
              primitive,
              duplicateVertices,
              vertices,
              indices,
              "TEXCOORD_" +
                  std::to_string(
                      encodedFeatureIDTexture.textureCoordinateSetIndex),
              gltfToUnrealTexCoordMap));
    } else {
      // Similar to feature ID attributes, we encode the unsigned integer vertex
      // ids as floats in the u-channel of a texture coordinate slot. If it ever
      // becomes possible to access the vertex ID through an Unreal material
      // node, this can be removed.
      uint32_t textureCoordinateIndex = gltfToUnrealTexCoordMap.size();
      gltfToUnrealTexCoordMap[-1] = textureCoordinateIndex;
      featuresMetadataTexcoordParameters.Emplace(
          SafeName,
          textureCoordinateIndex);
      if (duplicateVertices) {
        for (int64_t i = 0; i < indices.Num(); ++i) {
          FStaticMeshBuildVertex& vertex = vertices[i];
          uint32 vertexIndex = indices[i];
          vertex.UVs[textureCoordinateIndex] =
              TMeshVector2(static_cast<float>(vertexIndex), 0.0f);
        }
      } else {
        for (int64_t i = 0; i < vertices.Num(); ++i) {
          FStaticMeshBuildVertex& vertex = vertices[i];
          vertex.UVs[textureCoordinateIndex] =
              TMeshVector2(static_cast<float>(i), 0.0f);
        }
      }
    }
  }
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static void updateTextureCoordinatesForMetadata_DEPRECATED(
    const Model& model,
    const MeshPrimitive& primitive,
    bool duplicateVertices,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices,
    const CesiumEncodedMetadataUtility::EncodedMetadata& encodedMetadata,
    const CesiumEncodedMetadataUtility::EncodedMetadataPrimitive&
        encodedPrimitiveMetadata,
    const TArray<FCesiumFeatureIdAttribute>& featureIdAttributes,
    TMap<FString, uint32_t>& metadataTextureCoordinateParameters,
    std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::UpdateTextureCoordinatesForMetadata)

  for (const CesiumEncodedMetadataUtility::EncodedFeatureIdTexture&
           encodedFeatureIdTexture :
       encodedPrimitiveMetadata.encodedFeatureIdTextures) {
    metadataTextureCoordinateParameters.Emplace(
        encodedFeatureIdTexture.baseName + "UV",
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            vertices,
            indices,
            "TEXCOORD_" +
                std::to_string(
                    encodedFeatureIdTexture.textureCoordinateAttributeId),
            gltfToUnrealTexCoordMap));
  }

  for (const FString& featureTextureName :
       encodedPrimitiveMetadata.featureTextureNames) {
    const CesiumEncodedMetadataUtility::EncodedFeatureTexture*
        pEncodedFeatureTexture =
            encodedMetadata.encodedFeatureTextures.Find(featureTextureName);
    if (pEncodedFeatureTexture) {
      for (const CesiumEncodedMetadataUtility::EncodedFeatureTextureProperty&
               encodedProperty : pEncodedFeatureTexture->properties) {
        metadataTextureCoordinateParameters.Emplace(
            encodedProperty.baseName + "UV",
            updateTextureCoordinates(
                model,
                primitive,
                duplicateVertices,
                vertices,
                indices,
                "TEXCOORD_" + std::to_string(
                                  encodedProperty.textureCoordinateAttributeId),
                gltfToUnrealTexCoordMap));
      }
    }
  }

  const ExtensionExtMeshFeatures* pFeatures =
      primitive.getExtension<ExtensionExtMeshFeatures>();

  if (pFeatures) {
    for (const CesiumEncodedMetadataUtility::EncodedFeatureIdAttribute&
             encodedFeatureIdAttribute :
         encodedPrimitiveMetadata.encodedFeatureIdAttributes) {
      const FCesiumFeatureIdAttribute& featureIdAttribute =
          featureIdAttributes[encodedFeatureIdAttribute.index];

      int32_t attribute = featureIdAttribute.getAttributeIndex();
      std::string attributeName = "_FEATURE_ID_" + std::to_string(attribute);
      if (primitive.attributes.find(attributeName) ==
          primitive.attributes.end()) {
        continue;
      }

      // This was already validated when creating the EncodedFeatureIdSet.
      int32_t accessor = primitive.attributes.at(attributeName);

      uint32_t textureCoordinateIndex = gltfToUnrealTexCoordMap.size();
      gltfToUnrealTexCoordMap[accessor] = textureCoordinateIndex;
      metadataTextureCoordinateParameters.Emplace(
          encodedFeatureIdAttribute.name,
          textureCoordinateIndex);

      int64 vertexCount =
          UCesiumFeatureIdAttributeBlueprintLibrary::GetVertexCount(
              featureIdAttribute);

      // We encode unsigned integer feature ids as floats in the u-channel of
      // a texture coordinate slot.
      if (duplicateVertices) {
        for (int64_t i = 0; i < indices.Num(); ++i) {
          FStaticMeshBuildVertex& vertex = vertices[i];
          uint32 vertexIndex = indices[i];
          if (vertexIndex >= 0 && vertexIndex < vertexCount) {
            float featureId = static_cast<float>(
                UCesiumFeatureIdAttributeBlueprintLibrary::
                    GetFeatureIDForVertex(featureIdAttribute, vertexIndex));
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(featureId, 0.0f);
          } else {
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(0.0f, 0.0f);
          }
        }
      } else {
        for (int64_t i = 0; i < vertices.Num(); ++i) {
          FStaticMeshBuildVertex& vertex = vertices[i];
          if (i < vertexCount) {
            float featureId = static_cast<float>(
                UCesiumFeatureIdAttributeBlueprintLibrary::
                    GetFeatureIDForVertex(featureIdAttribute, i));
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(featureId, 0.0f);
          } else {
            vertex.UVs[textureCoordinateIndex] = TMeshVector2(0.0f, 0.0f);
          }
        }
      }
    }
  }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

static void loadPrimitiveFeaturesMetadata(
    LoadPrimitiveResult& primitiveResult,
    const CreatePrimitiveOptions& options,
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    bool duplicateVertices,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {

  ExtensionExtMeshFeatures* pFeatures =
      primitive.getExtension<ExtensionExtMeshFeatures>();

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

  const ExtensionMeshPrimitiveExtStructuralMetadata* pMetadata =
      primitive.getExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();

  const CreateGltfOptions::CreateModelOptions* pModelOptions =
      options.pMeshOptions->pNodeOptions->pModelOptions;
  const LoadGltfResult::LoadModelResult* pModelResult =
      options.pMeshOptions->pNodeOptions->pHalfConstructedModelResult;

  primitiveResult.Features =
      pFeatures ? FCesiumPrimitiveFeatures(model, primitive, *pFeatures)
                : FCesiumPrimitiveFeatures();
  primitiveResult.Metadata =
      pMetadata ? FCesiumPrimitiveMetadata(primitive, *pMetadata)
                : FCesiumPrimitiveMetadata();

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  primitiveResult.Metadata_DEPRECATED = FCesiumMetadataPrimitive{
      primitiveResult.Features,
      primitiveResult.Metadata,
      pModelResult->Metadata};

  createTexCoordAccessorsForFeaturesMetadata(
      model,
      primitive,
      primitiveResult.Features,
      primitiveResult.Metadata,
      pModelResult->Metadata,
      primitiveResult.TexCoordAccessorMap);

  const FCesiumFeaturesMetadataDescription* pFeaturesMetadataDescription =
      pModelOptions->pFeaturesMetadataDescription;

  // Check for deprecated metadata description
  const FMetadataDescription* pMetadataDescription_DEPRECATED =
      pModelOptions->pEncodedMetadataDescription_DEPRECATED;

  std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap =
      primitiveResult.GltfToUnrealTexCoordMap;

  if (pFeaturesMetadataDescription) {
    primitiveResult.EncodedFeatures =
        CesiumEncodedFeaturesMetadata::encodePrimitiveFeaturesAnyThreadPart(
            pFeaturesMetadataDescription->Features,
            primitiveResult.Features);

    primitiveResult.EncodedMetadata =
        CesiumEncodedFeaturesMetadata::encodePrimitiveMetadataAnyThreadPart(
            pFeaturesMetadataDescription->PrimitiveMetadata,
            primitiveResult.Metadata,
            pModelResult->Metadata);

    updateTextureCoordinatesForFeaturesMetadata(
        model,
        primitive,
        duplicateVertices,
        vertices,
        indices,
        primitiveResult.Features,
        primitiveResult.EncodedFeatures,
        primitiveResult.EncodedMetadata,
        pModelResult->EncodedMetadata,
        primitiveResult.FeaturesMetadataTexCoordParameters,
        gltfToUnrealTexCoordMap);
  } else if (pMetadataDescription_DEPRECATED) {
    primitiveResult.EncodedMetadata_DEPRECATED =
        CesiumEncodedMetadataUtility::encodeMetadataPrimitiveAnyThreadPart(
            *pMetadataDescription_DEPRECATED,
            primitiveResult.Metadata_DEPRECATED);

    updateTextureCoordinatesForMetadata_DEPRECATED(
        model,
        primitive,
        duplicateVertices,
        vertices,
        indices,
        *pModelResult->EncodedMetadata_DEPRECATED,
        *primitiveResult.EncodedMetadata_DEPRECATED,
        UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
            primitiveResult.Metadata_DEPRECATED),
        primitiveResult.FeaturesMetadataTexCoordParameters,
        gltfToUnrealTexCoordMap);
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

} // namespace

template <class TIndexAccessor>
static void loadPrimitive(
    LoadPrimitiveResult& primitiveResult,
    const glm::dmat4x4& transform,
    const CreatePrimitiveOptions& options,
    const Accessor& positionAccessor,
    const AccessorView<TMeshVector3>& positionView,
    const TIndexAccessor& indicesView,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadPrimitive<T>)

  Model& model = *options.pMeshOptions->pNodeOptions->pModelOptions->pModel;
  Mesh& mesh = *options.pMeshOptions->pMesh;
  MeshPrimitive& primitive = *options.pPrimitive;

  if (primitive.mode != MeshPrimitive::Mode::TRIANGLES &&
      primitive.mode != MeshPrimitive::Mode::TRIANGLE_STRIP &&
      primitive.mode != MeshPrimitive::Mode::POINTS) {
    // TODO: add support for other primitive types.
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Primitive mode %d is not supported"),
        primitive.mode);
    return;
  }

  std::string name = "glTF";

  auto urlIt = model.extras.find("Cesium3DTiles_TileUrl");
  if (urlIt != model.extras.end()) {
    name = urlIt->second.getStringOrDefault("glTF");
    name = constrainLength(name, 256);
  }

  auto meshIt = std::find_if(
      model.meshes.begin(),
      model.meshes.end(),
      [&mesh](const Mesh& candidate) { return &candidate == &mesh; });
  if (meshIt != model.meshes.end()) {
    int64_t meshIndex = meshIt - model.meshes.begin();
    name += " mesh " + std::to_string(meshIndex);
  }

  auto primitiveIt = std::find_if(
      mesh.primitives.begin(),
      mesh.primitives.end(),
      [&primitive](const MeshPrimitive& candidate) {
        return &candidate == &primitive;
      });
  if (primitiveIt != mesh.primitives.end()) {
    int64_t primitiveIndex = primitiveIt - mesh.primitives.begin();
    name += " primitive " + std::to_string(primitiveIndex);
  }

  primitiveResult.name = name;

  if (positionView.status() != AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("%s: Invalid position buffer"),
        UTF8_TO_TCHAR(name.c_str()));
    return;
  }

  if constexpr (IsAccessorView<TIndexAccessor>::value) {
    if (indicesView.status() != AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s: Invalid indices buffer"),
          UTF8_TO_TCHAR(name.c_str()));
      return;
    }
  }

  auto normalAccessorIt = primitive.attributes.find("NORMAL");
  AccessorView<TMeshVector3> normalAccessor;
  bool hasNormals = false;
  if (normalAccessorIt != primitive.attributes.end()) {
    int normalAccessorID = normalAccessorIt->second;
    normalAccessor = AccessorView<TMeshVector3>(model, normalAccessorID);
    hasNormals = normalAccessor.status() == AccessorViewStatus::Valid;
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
  const Material& material =
      materialID >= 0 && materialID < model.materials.size()
          ? model.materials[materialID]
          : defaultMaterial;

  primitiveResult.isUnlit =
      material.hasExtension<ExtensionKhrMaterialsUnlit>() &&
      !options.pMeshOptions->pNodeOptions->pModelOptions
           ->ignoreKhrMaterialsUnlit;

  // We can't calculate flat normals for points or lines, so we have to force
  // them to be unlit if no normals are specified. Otherwise this causes a
  // crash when attempting to calculate flat normals.
  bool isTriangles = primitive.mode == MeshPrimitive::Mode::TRIANGLES ||
                     primitive.mode == MeshPrimitive::Mode::TRIANGLE_FAN ||
                     primitive.mode == MeshPrimitive::Mode::TRIANGLE_STRIP;

  if (!isTriangles && !hasNormals) {
    primitiveResult.isUnlit = true;
  }

  const MaterialPBRMetallicRoughness& pbrMetallicRoughness =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  bool hasNormalMap = material.normalTexture.has_value();
  if (hasNormalMap) {
    const CesiumGltf::Texture* pTexture =
        Model::getSafe(&model.textures, material.normalTexture->index);
    hasNormalMap = pTexture != nullptr &&
                   Model::getSafe(&model.images, pTexture->source) != nullptr;
  }

  bool needsTangents =
      hasNormalMap ||
      options.pMeshOptions->pNodeOptions->pModelOptions->alwaysIncludeTangents;

  bool hasTangents = false;
  auto tangentAccessorIt = primitive.attributes.find("TANGENT");
  AccessorView<TMeshVector4> tangentAccessor;
  if (tangentAccessorIt != primitive.attributes.end()) {
    int tangentAccessorID = tangentAccessorIt->second;
    tangentAccessor = AccessorView<TMeshVector4>(model, tangentAccessorID);
    hasTangents = tangentAccessor.status() == AccessorViewStatus::Valid;
    if (!hasTangents) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s: Invalid tangent buffer."),
          UTF8_TO_TCHAR(name.c_str()));
    }
  }

  applyWaterMask(model, primitive, primitiveResult, textureResources);

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
      for (int32_t i = 0; i < positionView.size(); ++i) {
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

  TArray<uint32> indices;
  if (primitive.mode == MeshPrimitive::Mode::TRIANGLES ||
      primitive.mode == MeshPrimitive::Mode::POINTS) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyIndices)
    indices.SetNum(static_cast<TArray<uint32>::SizeType>(indicesView.size()));

    for (int32 i = 0; i < indicesView.size(); ++i) {
      indices[i] = indicesView[i];
    }
  } else {
    // assume TRIANGLE_STRIP because all others are rejected earlier.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyIndices)
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
  }

  // If we don't have normals, the gltf spec prescribes that the client
  // implementation must generate flat normals, which requires duplicating
  // vertices shared by multiple triangles. If we don't have tangents, but
  // need them, we need to use a tangent space generation algorithm which
  // requires duplicated vertices.
  bool duplicateVertices = !hasNormals || (needsTangents && !hasTangents);
  duplicateVertices =
      duplicateVertices && primitive.mode != MeshPrimitive::Mode::POINTS;

  TArray<FStaticMeshBuildVertex> StaticMeshBuildVertices;
  StaticMeshBuildVertices.SetNum(
      duplicateVertices ? indices.Num()
                        : static_cast<int>(positionView.size()));

  {
    if (duplicateVertices) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyDuplicatedPositions)
      for (int i = 0; i < indices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        uint32 vertexIndex = indices[i];
        const TMeshVector3& pos = positionView[vertexIndex];
        vertex.Position.X = pos.X;
        vertex.Position.Y = -pos.Y;
        vertex.Position.Z = pos.Z;
        vertex.UVs[0] = TMeshVector2(0.0f, 0.0f);
        vertex.UVs[2] = TMeshVector2(0.0f, 0.0f);
        RenderData->Bounds.SphereRadius = FMath::Max(
            (FVector(vertex.Position) - RenderData->Bounds.Origin).Size(),
            RenderData->Bounds.SphereRadius);
      }
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyPositions)
      for (int i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        const TMeshVector3& pos = positionView[i];
        vertex.Position.X = pos.X;
        vertex.Position.Y = -pos.Y;
        vertex.Position.Z = pos.Z;
        vertex.UVs[0] = TMeshVector2(0.0f, 0.0f);
        vertex.UVs[2] = TMeshVector2(0.0f, 0.0f);
        RenderData->Bounds.SphereRadius = FMath::Max(
            (FVector(vertex.Position) - RenderData->Bounds.Origin).Size(),
            RenderData->Bounds.SphereRadius);
      }
    }
  }

  bool hasVertexColors = false;

  auto colorAccessorIt = primitive.attributes.find("COLOR_0");
  if (colorAccessorIt != primitive.attributes.end()) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyVertexColors)
    int colorAccessorID = colorAccessorIt->second;
    hasVertexColors = createAccessorView(
        model,
        colorAccessorID,
        ColorVisitor{duplicateVertices, StaticMeshBuildVertices, indices});
  }

  LODResources.bHasColorVertexData = hasVertexColors;

  // We need to copy the texture coordinates associated with each texture (if
  // any) into the the appropriate UVs slot in FStaticMeshBuildVertex.

  std::unordered_map<int32_t, uint32_t>& gltfToUnrealTexCoordMap =
      primitiveResult.GltfToUnrealTexCoordMap;

  // This must be done before material textures are loaded, in case any of the
  // material textures are also used for features + metadata.
  loadPrimitiveFeaturesMetadata(
      primitiveResult,
      options,
      model,
      primitive,
      duplicateVertices,
      StaticMeshBuildVertices,
      indices,
      textureResources);

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadTextures)
    primitiveResult.baseColorTexture = loadTexture(
        model,
        pbrMetallicRoughness.baseColorTexture,
        true,
        textureResources);
    primitiveResult.metallicRoughnessTexture = loadTexture(
        model,
        pbrMetallicRoughness.metallicRoughnessTexture,
        false,
        textureResources);
    primitiveResult.normalTexture =
        loadTexture(model, material.normalTexture, false, textureResources);
    primitiveResult.occlusionTexture =
        loadTexture(model, material.occlusionTexture, false, textureResources);
    primitiveResult.emissiveTexture =
        loadTexture(model, material.emissiveTexture, true, textureResources);
  }

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::UpdateTextureCoordinates)

    primitiveResult
        .textureCoordinateParameters["baseColorTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            pbrMetallicRoughness.baseColorTexture,
            gltfToUnrealTexCoordMap);
    primitiveResult.textureCoordinateParameters
        ["metallicRoughnessTextureCoordinateIndex"] = updateTextureCoordinates(
        model,
        primitive,
        duplicateVertices,
        StaticMeshBuildVertices,
        indices,
        pbrMetallicRoughness.metallicRoughnessTexture,
        gltfToUnrealTexCoordMap);
    primitiveResult
        .textureCoordinateParameters["normalTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            material.normalTexture,
            gltfToUnrealTexCoordMap);
    primitiveResult
        .textureCoordinateParameters["occlusionTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            material.occlusionTexture,
            gltfToUnrealTexCoordMap);
    primitiveResult
        .textureCoordinateParameters["emissiveTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            material.emissiveTexture,
            gltfToUnrealTexCoordMap);

    for (size_t i = 0;
         i < primitiveResult.overlayTextureCoordinateIDToUVIndex.size();
         ++i) {
      std::string attributeName = "_CESIUMOVERLAY_" + std::to_string(i);
      auto overlayIt = primitive.attributes.find(attributeName);
      if (overlayIt != primitive.attributes.end()) {
        primitiveResult.overlayTextureCoordinateIDToUVIndex[i] =
            updateTextureCoordinates(
                model,
                primitive,
                duplicateVertices,
                StaticMeshBuildVertices,
                indices,
                attributeName,
                gltfToUnrealTexCoordMap);
      } else {
        primitiveResult.overlayTextureCoordinateIDToUVIndex[i] = 0;
      }
    }
  }

  // TangentX: Tangent
  // TangentY: Bi-tangent
  // TangentZ: Normal

  if (hasNormals) {
    if (duplicateVertices) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyNormalsForDuplicatedVertices)
      for (int i = 0; i < indices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        uint32 vertexIndex = indices[i];
        vertex.TangentX = TMeshVector3(0.0f, 0.0f, 0.0f);
        vertex.TangentY = TMeshVector3(0.0f, 0.0f, 0.0f);
        const TMeshVector3& normal = normalAccessor[vertexIndex];
        vertex.TangentZ.X = normal.X;
        vertex.TangentZ.Y = -normal.Y;
        vertex.TangentZ.Z = normal.Z;
      }
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyNormals)
      for (int i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        vertex.TangentX = TMeshVector3(0.0f, 0.0f, 0.0f);
        vertex.TangentY = TMeshVector3(0.0f, 0.0f, 0.0f);
        const TMeshVector3& normal = normalAccessor[i];
        vertex.TangentZ.X = normal.X;
        vertex.TangentZ.Y = -normal.Y;
        vertex.TangentZ.Z = normal.Z;
      }
    }
  } else {
    if (primitiveResult.isUnlit) {
      glm::dvec3 ecefCenter = glm::dvec3(
          transform *
          glm::dvec4(VecMath::createVector3D(RenderData->Bounds.Origin), 1.0));
      TMeshVector3 upDir = TMeshVector3(VecMath::createVector(
          glm::affineInverse(transform) *
          glm::dvec4(
              CesiumGeospatial::Ellipsoid::WGS84.geodeticSurfaceNormal(
                  glm::dvec3(ecefCenter)),
              0.0)));
      upDir.Y *= -1;
      setUniformNormals(StaticMeshBuildVertices, upDir);
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ComputeFlatNormals)
      computeFlatNormals(StaticMeshBuildVertices);
    }
  }

  if (hasTangents) {
    if (duplicateVertices) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyTangentsForDuplicatedVertices)
      for (int i = 0; i < indices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        uint32 vertexIndex = indices[i];
        const TMeshVector4& tangent = tangentAccessor[vertexIndex];
        vertex.TangentX.X = tangent.X;
        vertex.TangentX.Y = -tangent.Y;
        vertex.TangentX.Z = tangent.Z;
        vertex.TangentY =
            TMeshVector3::CrossProduct(vertex.TangentZ, vertex.TangentX) *
            tangent.W;
      }
    } else {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CopyTangents)
      for (int i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        const TMeshVector4& tangent = tangentAccessor[i];
        vertex.TangentX = tangent;
        vertex.TangentX.X = tangent.X;
        vertex.TangentX.Y = -tangent.Y;
        vertex.TangentX.Z = tangent.Z;
        vertex.TangentY =
            TMeshVector3::CrossProduct(vertex.TangentZ, vertex.TangentX) *
            tangent.W;
      }
    }
  }

  if (needsTangents && !hasTangents) {
    // Use mikktspace to calculate the tangents.
    // Note that this assumes normals and UVs are already populated.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ComputeTangents)
    computeTangentSpace(StaticMeshBuildVertices);
  }

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::InitBuffers)

    // Set to full precision (32-bit) UVs. This is especially important for
    // metadata because integer feature IDs can and will lose meaningful
    // precision when using 16-bit floats.
    LODResources.VertexBuffers.StaticMeshVertexBuffer.SetUseFullPrecisionUVs(
        true);

    LODResources.VertexBuffers.PositionVertexBuffer.Init(
        StaticMeshBuildVertices,
        false);

    FColorVertexBuffer& ColorVertexBuffer =
        LODResources.VertexBuffers.ColorVertexBuffer;
    if (hasVertexColors) {
      ColorVertexBuffer.Init(StaticMeshBuildVertices, false);
    }

    LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(
        StaticMeshBuildVertices,
        gltfToUnrealTexCoordMap.size() == 0 ? 1
                                            : gltfToUnrealTexCoordMap.size(),
        false);
  }

  FStaticMeshSectionArray& Sections = LODResources.Sections;
  FStaticMeshSection& section = Sections.AddDefaulted_GetRef();
  // This will be ignored if the primitive contains points.
  section.NumTriangles = indices.Num() / 3;
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = StaticMeshBuildVertices.Num() - 1;
  section.bEnableCollision = primitive.mode != MeshPrimitive::Mode::POINTS;
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
        StaticMeshBuildVertices.Num() >= std::numeric_limits<uint16>::max()
            ? EIndexBufferStride::Type::Force32Bit
            : EIndexBufferStride::Type::Force16Bit);
  }

  LODResources.bHasDepthOnlyIndices = false;
  LODResources.bHasReversedIndices = false;
  LODResources.bHasReversedDepthOnlyIndices = false;

  primitiveResult.pModel = &model;
  primitiveResult.pMeshPrimitive = &primitive;
  primitiveResult.RenderData = std::move(RenderData);
  primitiveResult.pMaterial = &material;
  primitiveResult.pCollisionMesh = nullptr;

  // This matrix converts from right-handed Z-up to Unreal
  // left-handed Z-up by flipping the Y axis. It effectively undoes the Y-axis
  // flipping that we did when creating the mesh in the first place. This is
  // necessary to work around a problem in UE 5.1 where negatively-scaled
  // meshes don't work correctly for collision. See
  // https://github.com/CesiumGS/cesium-unreal/pull/1126
  static constexpr glm::dmat4 yInvertMatrix = {
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

  primitiveResult.transform = transform * yInvertMatrix;

  if (primitive.mode != MeshPrimitive::Mode::POINTS &&
      options.pMeshOptions->pNodeOptions->pModelOptions->createPhysicsMeshes) {
    if (StaticMeshBuildVertices.Num() != 0 && indices.Num() != 0) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ChaosCook)
      primitiveResult.pCollisionMesh =
          StaticMeshBuildVertices.Num() < TNumericLimits<uint16>::Max()
              ? BuildChaosTriangleMeshes<uint16>(
                    StaticMeshBuildVertices,
                    indices)
              : BuildChaosTriangleMeshes<int32>(
                    StaticMeshBuildVertices,
                    indices);
    }
  }
}

static void loadIndexedPrimitive(
    LoadPrimitiveResult& primitiveResult,
    const glm::dmat4x4& transform,
    const CreatePrimitiveOptions& options,
    const Accessor& positionAccessor,
    const AccessorView<TMeshVector3>& positionView,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {
  const Model& model =
      *options.pMeshOptions->pNodeOptions->pModelOptions->pModel;
  const MeshPrimitive& primitive = *options.pPrimitive;

  const Accessor& indexAccessorGltf = model.accessors[primitive.indices];
  if (indexAccessorGltf.componentType ==
      Accessor::ComponentType::UNSIGNED_BYTE) {
    AccessorView<uint8_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        primitiveResult,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor,
        textureResources);
    primitiveResult.IndexAccessor = indexAccessor;
  } else if (
      indexAccessorGltf.componentType ==
      Accessor::ComponentType::UNSIGNED_SHORT) {
    AccessorView<uint16_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        primitiveResult,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor,
        textureResources);
    primitiveResult.IndexAccessor = indexAccessor;
  } else if (
      indexAccessorGltf.componentType ==
      Accessor::ComponentType::UNSIGNED_INT) {
    AccessorView<uint32_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        primitiveResult,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor,
        textureResources);
    primitiveResult.IndexAccessor = indexAccessor;
  } else {
    UE_LOG(
        LogCesium,
        VeryVerbose,
        TEXT(
            "Skip loading primitive due to invalid component type in its index accessor."));
  }
}

static void loadPrimitive(
    LoadPrimitiveResult& result,
    const glm::dmat4x4& transform,
    const CreatePrimitiveOptions& options,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadPrimitive)

  const Model& model =
      *options.pMeshOptions->pNodeOptions->pModelOptions->pModel;
  const MeshPrimitive& primitive = *options.pPrimitive;

  auto positionAccessorIt = primitive.attributes.find("POSITION");
  if (positionAccessorIt == primitive.attributes.end()) {
    // This primitive doesn't have a POSITION semantic, ignore it.
    return;
  }

  int positionAccessorID = positionAccessorIt->second;
  const Accessor* pPositionAccessor =
      Model::getSafe(&model.accessors, positionAccessorID);
  if (!pPositionAccessor) {
    // Position accessor does not exist, so ignore this primitive.
    return;
  }

  AccessorView<TMeshVector3> positionView(model, *pPositionAccessor);

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
        textureResources);
  } else {
    loadIndexedPrimitive(
        result,
        transform,
        options,
        *pPositionAccessor,
        positionView,
        textureResources);
  }
  result.PositionAccessor = std::move(positionView);
}

static void loadMesh(
    std::optional<LoadMeshResult>& result,
    const glm::dmat4x4& transform,
    const CreateMeshOptions& options,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadMesh)

  const Model& model = *options.pNodeOptions->pModelOptions->pModel;
  Mesh& mesh = *options.pMesh;

  result = LoadMeshResult();
  result->primitiveResults.reserve(mesh.primitives.size());
  for (CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
    CreatePrimitiveOptions primitiveOptions = {&options, &*result, &primitive};
    auto& primitiveResult = result->primitiveResults.emplace_back();
    loadPrimitive(
        primitiveResult,
        transform,
        primitiveOptions,
        textureResources);

    // if it doesn't have render data, then it can't be loaded
    if (!primitiveResult.RenderData) {
      result->primitiveResults.pop_back();
    }
  }
}

static void loadNode(
    std::vector<LoadNodeResult>& loadNodeResults,
    const glm::dmat4x4& transform,
    const CreateNodeOptions& options,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {

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

  Model& model = *options.pModelOptions->pModel;
  const Node& node = *options.pNode;

  LoadNodeResult& result = loadNodeResults.emplace_back();

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
    CreateMeshOptions meshOptions = {&options, &result, &model.meshes[meshId]};
    loadMesh(result.meshResult, nodeTransform, meshOptions, textureResources);
  }

  for (int childNodeId : node.children) {
    if (childNodeId >= 0 && childNodeId < model.nodes.size()) {
      CreateNodeOptions childNodeOptions = {
          options.pModelOptions,
          options.pHalfConstructedModelResult,
          &model.nodes[childNodeId]};
      loadNode(
          loadNodeResults,
          nodeTransform,
          childNodeOptions,
          textureResources);
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
void applyGltfUpAxisTransform(const Model& model, glm::dmat4x4& rootTransform) {

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
        VeryVerbose,
        TEXT("Unknown gltfUpAxis value: {}"),
        gltfUpAxisValue);
  }
}

} // namespace

static void
loadModelMetadata(LoadModelResult& result, const CreateModelOptions& options) {
  Model& model = *options.pModel;

  ExtensionModelExtStructuralMetadata* pModelMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
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
        const ExtensionMeshPrimitiveExtStructuralMetadata* pPrimitiveMetadata =
            primitive
                .getExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
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
        CesiumEncodedFeaturesMetadata::encodeModelMetadataAnyThreadPart(
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

static void loadModelAnyThreadPart(
    LoadModelResult& result,
    const glm::dmat4x4& transform,
    const CreateModelOptions& options,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadModelAnyThreadPart)

  Model& model = *options.pModel;

  // Generate mipmaps if needed.
  // An image needs mipmaps generated for it if:
  // 1. It is used by a Texture that has a Sampler with a mipmap filtering
  // mode, and
  // 2. It does not already have mipmaps.
  // It's ok if an image has mipmaps even if not all textures will use them.
  // There's no reason to have two RHI textures, one with and one without
  // mips.
  for (const Texture& texture : model.textures) {
    const Sampler& sampler = model.getSafe(model.samplers, texture.sampler);

    bool needsMipmaps;
    switch (sampler.minFilter.value_or(
        CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR)) {
    case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
    case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
    case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
    case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
      needsMipmaps = true;
      break;
    default: // LINEAR and NEAREST
      needsMipmaps = false;
      break;
    }

    if (!needsMipmaps)
      continue;

    Image* pImage = model.getSafe(&model.images, texture.source);
    if (!pImage || pImage->cesium.pixelData.empty())
      continue;

    std::optional<std::string> errorMessage =
        CesiumGltfReader::GltfReader::generateMipMaps(pImage->cesium);
    if (errorMessage) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s"),
          UTF8_TO_TCHAR(errorMessage->c_str()));
    }
  }

  loadModelMetadata(result, options);

  glm::dmat4x4 rootTransform = transform;

  {
    rootTransform =
        CesiumGltfContent::GltfUtilities::applyRtcCenter(model, rootTransform);
    applyGltfUpAxisTransform(model, rootTransform);
  }

  if (model.scene >= 0 && model.scene < model.scenes.size()) {
    // Show the default scene
    const Scene& defaultScene = model.scenes[model.scene];
    for (int nodeId : defaultScene.nodes) {
      CreateNodeOptions nodeOptions = {&options, &result, &model.nodes[nodeId]};
      loadNode(
          result.nodeResults,
          rootTransform,
          nodeOptions,
          textureResources);
    }
  } else if (model.scenes.size() > 0) {
    // There's no default, so show the first scene
    const Scene& defaultScene = model.scenes[0];
    for (int nodeId : defaultScene.nodes) {
      CreateNodeOptions nodeOptions = {&options, &result, &model.nodes[nodeId]};
      loadNode(
          result.nodeResults,
          rootTransform,
          nodeOptions,
          textureResources);
    }
  } else if (model.nodes.size() > 0) {
    // No scenes at all, use the first node as the root node.
    CreateNodeOptions nodeOptions = {&options, &result, &model.nodes[0]};
    loadNode(result.nodeResults, rootTransform, nodeOptions, textureResources);
  } else if (model.meshes.size() > 0) {
    // No nodes either, show all the meshes.
    for (Mesh& mesh : model.meshes) {
      CreateNodeOptions dummyNodeOptions = {&options, &result, nullptr};
      LoadNodeResult& dummyNodeResult = result.nodeResults.emplace_back();
      CreateMeshOptions meshOptions = {
          &dummyNodeOptions,
          &dummyNodeResult,
          &mesh};
      loadMesh(
          dummyNodeResult.meshResult,
          rootTransform,
          meshOptions,
          textureResources);
    }
  }
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

  pMaterial->SetTextureParameterValueByInfo(info, pTexture->pUnrealTexture);

  return true;
}

#pragma region Material Parameter setters

static void SetGltfParameterValues(
    CesiumGltf::Model& model,
    LoadPrimitiveResult& loadResult,
    const Material& material,
    const MaterialPBRMetallicRoughness& pbr,
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

  KhrTextureTransform textureTransform;
  FLinearColor baseColorMetallicRoughnessRotation(0.0f, 1.0f, 0.0f, 1.0f);
  const ExtensionKhrTextureTransform* pBaseColorTextureTransform =
      pbr.baseColorTexture
          ? pbr.baseColorTexture->getExtension<ExtensionKhrTextureTransform>()
          : nullptr;

  if (pBaseColorTextureTransform) {
    textureTransform = KhrTextureTransform(*pBaseColorTextureTransform);
    if (textureTransform.status() == KhrTextureTransformStatus::Valid) {
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

  const ExtensionKhrTextureTransform* pMetallicRoughnessTextureTransform =
      pbr.metallicRoughnessTexture
          ? pbr.metallicRoughnessTexture
                ->getExtension<ExtensionKhrTextureTransform>()
          : nullptr;

  if (pMetallicRoughnessTextureTransform) {
    textureTransform = KhrTextureTransform(*pMetallicRoughnessTextureTransform);
    if (textureTransform.status() == KhrTextureTransformStatus::Valid) {
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

  const ExtensionKhrTextureTransform* pEmissiveTextureTransform =
      material.emissiveTexture
          ? material.emissiveTexture
                ->getExtension<ExtensionKhrTextureTransform>()
          : nullptr;

  if (pEmissiveTextureTransform) {
    textureTransform = KhrTextureTransform(*pEmissiveTextureTransform);
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

  const ExtensionKhrTextureTransform* pNormalTextureTransform =
      material.normalTexture
          ? material.normalTexture->getExtension<ExtensionKhrTextureTransform>()
          : nullptr;

  if (pNormalTextureTransform) {
    textureTransform = KhrTextureTransform(*pNormalTextureTransform);
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

  const ExtensionKhrTextureTransform* pOcclusionTransform =
      material.occlusionTexture
          ? material.occlusionTexture
                ->getExtension<ExtensionKhrTextureTransform>()
          : nullptr;

  if (pOcclusionTransform) {
    textureTransform = KhrTextureTransform(*pOcclusionTransform);
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
    LoadPrimitiveResult& loadResult,
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

static void SetFeatureIdTextureParameterValues(
    const CesiumEncodedFeaturesMetadata::EncodedFeatureIdTexture&
        encodedFeatureIdTexture,
    const FString& name,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  pMaterial->SetTextureParameterValueByInfo(
      FMaterialParameterInfo(
          FName(name + CesiumEncodedFeaturesMetadata::MaterialTextureSuffix),
          association,
          index),
      encodedFeatureIdTexture.pTexture->pTexture->pUnrealTexture);

  size_t numChannels = encodedFeatureIdTexture.channels.size();
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo(
          FName(
              name + CesiumEncodedFeaturesMetadata::MaterialNumChannelsSuffix),
          association,
          index),
      static_cast<float>(numChannels));

  std::vector<float> channelsAsFloats{0.0f, 0.0f, 0.0f, 0.0f};
  for (size_t i = 0; i < numChannels; i++) {
    channelsAsFloats[i] =
        static_cast<float>(encodedFeatureIdTexture.channels[i]);
  }

  FLinearColor channels{
      channelsAsFloats[0],
      channelsAsFloats[1],
      channelsAsFloats[2],
      channelsAsFloats[3],
  };

  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          FName(name + CesiumEncodedFeaturesMetadata::MaterialChannelsSuffix),
          association,
          index),
      channels);

  if (!encodedFeatureIdTexture.textureTransform) {
    return;
  }

  glm::dvec2 scale = encodedFeatureIdTexture.textureTransform->scale();
  glm::dvec2 offset = encodedFeatureIdTexture.textureTransform->offset();

  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          FName(
              name +
              CesiumEncodedFeaturesMetadata::MaterialTextureScaleOffsetSuffix),
          association,
          index),
      FLinearColor(scale[0], scale[1], offset[0], offset[1]));

  glm::dvec2 rotation =
      encodedFeatureIdTexture.textureTransform->rotationSineCosine();
  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          FName(
              name +
              CesiumEncodedFeaturesMetadata::MaterialTextureRotationSuffix),
          association,
          index),
      FLinearColor(rotation[0], rotation[1], 0.0f, 1.0f));
}

static void SetPropertyParameterValue(
    const FString& name,
    ECesiumEncodedMetadataType type,
    const FCesiumMetadataValue& value,
    const float defaultValue,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  if (type == ECesiumEncodedMetadataType::Scalar) {
    pMaterial->SetScalarParameterValueByInfo(
        FMaterialParameterInfo(FName(name), association, index),
        UCesiumMetadataValueBlueprintLibrary::GetFloat(value, defaultValue));
  } else if (
      type == ECesiumEncodedMetadataType::Vec2 ||
      type == ECesiumEncodedMetadataType::Vec3 ||
      type == ECesiumEncodedMetadataType::Vec4) {
    FVector4 vector4Value = UCesiumMetadataValueBlueprintLibrary::GetVector4(
        value,
        FVector4(defaultValue, defaultValue, defaultValue, defaultValue));

    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(FName(name), association, index),
        FLinearColor(
            static_cast<float>(vector4Value.X),
            static_cast<float>(vector4Value.Y),
            static_cast<float>(vector4Value.Z),
            static_cast<float>(vector4Value.W)));
  }
}

static void SetPropertyTableParameterValues(
    const CesiumEncodedFeaturesMetadata::EncodedPropertyTable&
        encodedPropertyTable,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  for (const CesiumEncodedFeaturesMetadata::EncodedPropertyTableProperty&
           encodedProperty : encodedPropertyTable.properties) {
    FString fullPropertyName =
        CesiumEncodedFeaturesMetadata::getMaterialNameForPropertyTableProperty(
            encodedPropertyTable.name,
            encodedProperty.name);

    if (encodedProperty.pTexture) {
      pMaterial->SetTextureParameterValueByInfo(
          FMaterialParameterInfo(FName(fullPropertyName), association, index),
          encodedProperty.pTexture->pTexture->pUnrealTexture);
    }

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(
            encodedProperty.offset)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyOffsetSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          0.0f,
          pMaterial,
          association,
          index);
    }

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(encodedProperty.scale)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyScaleSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          1.0f,
          pMaterial,
          association,
          index);
    }

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(
            encodedProperty.noData)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyNoDataSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          0.0f,
          pMaterial,
          association,
          index);
    }

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(
            encodedProperty.defaultValue)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyDefaultValueSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          0.0f,
          pMaterial,
          association,
          index);

      FString hasValueName = fullPropertyName =
          CesiumEncodedFeaturesMetadata::MaterialPropertyHasValueSuffix;
      pMaterial->SetScalarParameterValueByInfo(
          FMaterialParameterInfo(FName(hasValueName), association, index),
          encodedProperty.pTexture ? 1.0 : 0.0);
    }
  }
}

static void SetPropertyTextureParameterValues(
    const CesiumEncodedFeaturesMetadata::EncodedPropertyTexture&
        encodedPropertyTexture,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index) {
  for (const CesiumEncodedFeaturesMetadata::EncodedPropertyTextureProperty&
           encodedProperty : encodedPropertyTexture.properties) {
    FString fullPropertyName = CesiumEncodedFeaturesMetadata::
        getMaterialNameForPropertyTextureProperty(
            encodedPropertyTexture.name,
            encodedProperty.name);

    if (encodedProperty.pTexture) {
      pMaterial->SetTextureParameterValueByInfo(
          FMaterialParameterInfo(FName(fullPropertyName), association, index),
          encodedProperty.pTexture->pTexture->pUnrealTexture);
    }

    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            FName(
                fullPropertyName +
                CesiumEncodedFeaturesMetadata::MaterialChannelsSuffix),
            association,
            index),
        FLinearColor(
            encodedProperty.channels[0],
            encodedProperty.channels[1],
            encodedProperty.channels[2],
            encodedProperty.channels[3]));

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(
            encodedProperty.offset)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyOffsetSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          0.0f,
          pMaterial,
          association,
          index);
    }

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(encodedProperty.scale)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyScaleSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          1.0f,
          pMaterial,
          association,
          index);
    }

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(
            encodedProperty.noData)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyNoDataSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          0.0f,
          pMaterial,
          association,
          index);
    }

    if (!UCesiumMetadataValueBlueprintLibrary::IsEmpty(
            encodedProperty.defaultValue)) {
      FString parameterName =
          fullPropertyName +
          CesiumEncodedFeaturesMetadata::MaterialPropertyDefaultValueSuffix;
      SetPropertyParameterValue(
          parameterName,
          encodedProperty.type,
          encodedProperty.offset,
          0.0f,
          pMaterial,
          association,
          index);

      FString hasValueName = fullPropertyName =
          CesiumEncodedFeaturesMetadata::MaterialPropertyHasValueSuffix;
      pMaterial->SetScalarParameterValueByInfo(
          FMaterialParameterInfo(FName(hasValueName), association, index),
          encodedProperty.pTexture ? 1.0 : 0.0);
    }

    if (!encodedProperty.textureTransform) {
      continue;
    }

    glm::dvec2 scale = encodedProperty.textureTransform->scale();
    glm::dvec2 offset = encodedProperty.textureTransform->offset();

    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            FName(
                fullPropertyName + CesiumEncodedFeaturesMetadata::
                                       MaterialTextureScaleOffsetSuffix),
            association,
            index),
        FLinearColor(scale[0], scale[1], offset[0], offset[1]));

    glm::dvec2 rotation =
        encodedProperty.textureTransform->rotationSineCosine();
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            FName(
                fullPropertyName +
                CesiumEncodedFeaturesMetadata::MaterialTextureRotationSuffix),
            association,
            index),
        FLinearColor(rotation[0], rotation[1], 0.0f, 1.0f));
  }
}

static void SetFeaturesMetadataParameterValues(
    const CesiumGltf::Model& model,
    UCesiumGltfComponent& gltfComponent,
    LoadPrimitiveResult& loadResult,
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
    for (CesiumEncodedFeaturesMetadata::EncodedFeatureIdSet&
             encodedFeatureIdSet : loadResult.EncodedFeatures.featureIdSets) {
      FString SafeName = CesiumEncodedFeaturesMetadata::createHlslSafeName(
          encodedFeatureIdSet.name);
      if (encodedFeatureIdSet.nullFeatureId) {
        pMaterial->SetScalarParameterValueByInfo(
            FMaterialParameterInfo(
                FName(
                    SafeName +
                    CesiumEncodedFeaturesMetadata::MaterialNullFeatureIdSuffix),
                association,
                index),
            static_cast<float>(*encodedFeatureIdSet.nullFeatureId));
      }

      if (encodedFeatureIdSet.texture) {
        SetFeatureIdTextureParameterValues(
            *encodedFeatureIdSet.texture,
            SafeName,
            pMaterial,
            association,
            index);
      }
    }

    for (const CesiumEncodedFeaturesMetadata::EncodedPropertyTexture&
             propertyTexture : gltfComponent.EncodedMetadata.propertyTextures) {
      SetPropertyTextureParameterValues(
          propertyTexture,
          pMaterial,
          association,
          index);
    }

    for (const CesiumEncodedFeaturesMetadata::EncodedPropertyTable&
             propertyTable : gltfComponent.EncodedMetadata.propertyTables) {
      SetPropertyTableParameterValues(
          propertyTable,
          pMaterial,
          association,
          index);
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
        encodedProperty.pTexture->pTexture->pUnrealTexture);
  }
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static void SetMetadataParameterValues_DEPRECATED(
    const CesiumGltf::Model& model,
    UCesiumGltfComponent& gltfComponent,
    LoadPrimitiveResult& loadResult,
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
            encodedProperty.pTexture->pTexture->pUnrealTexture);

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
        encodedFeatureIdTexture.pTexture->pTexture->pUnrealTexture);

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

static void loadPrimitiveGameThreadPart(
    CesiumGltf::Model& model,
    UCesiumGltfComponent* pGltf,
    LoadPrimitiveResult& loadResult,
    const glm::dmat4x4& cesiumToUnrealTransform,
    const Cesium3DTilesSelection::Tile& tile,
    bool createNavCollision,
    ACesium3DTileset* pTilesetActor) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LoadPrimitive)

  const Cesium3DTilesSelection::BoundingVolume& boundingVolume =
      tile.getContentBoundingVolume().value_or(tile.getBoundingVolume());

  FName meshName = createSafeName(loadResult.name, "");
  UCesiumGltfPrimitiveComponent* pMesh;
  if (loadResult.pMeshPrimitive->mode == MeshPrimitive::Mode::POINTS) {
    UCesiumGltfPointsComponent* pPointMesh =
        NewObject<UCesiumGltfPointsComponent>(pGltf, meshName);
    pPointMesh->UsesAdditiveRefinement =
        tile.getRefine() == Cesium3DTilesSelection::TileRefine::Add;
    pPointMesh->GeometricError = static_cast<float>(tile.getGeometricError());
    pPointMesh->Dimensions = loadResult.dimensions;
    pMesh = pPointMesh;
  } else {
    pMesh = NewObject<UCesiumGltfPrimitiveComponent>(pGltf, meshName);
  }

  UStaticMesh* pStaticMesh;
  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetupMesh)

    pMesh->pTilesetActor = pTilesetActor;
    pMesh->overlayTextureCoordinateIDToUVIndex =
        loadResult.overlayTextureCoordinateIDToUVIndex;
    pMesh->GltfToUnrealTexCoordMap =
        std::move(loadResult.GltfToUnrealTexCoordMap);
    pMesh->TexCoordAccessorMap = std::move(loadResult.TexCoordAccessorMap);
    pMesh->PositionAccessor = std::move(loadResult.PositionAccessor);
    pMesh->IndexAccessor = std::move(loadResult.IndexAccessor);
    pMesh->HighPrecisionNodeTransform = loadResult.transform;
    pMesh->UpdateTransformFromCesium(cesiumToUnrealTransform);

    pMesh->bUseDefaultCollision = false;
    pMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    pMesh->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    pMesh->pModel = loadResult.pModel;
    pMesh->pMeshPrimitive = loadResult.pMeshPrimitive;
    pMesh->boundingVolume = boundingVolume;
    pMesh->SetRenderCustomDepth(pGltf->CustomDepthParameters.RenderCustomDepth);
    pMesh->SetCustomDepthStencilWriteMask(
        pGltf->CustomDepthParameters.CustomDepthStencilWriteMask);
    pMesh->SetCustomDepthStencilValue(
        pGltf->CustomDepthParameters.CustomDepthStencilValue);
    if (loadResult.isUnlit) {
      pMesh->bCastDynamicShadow = false;
    }

    pStaticMesh = NewObject<UStaticMesh>(pMesh, meshName);
    pMesh->SetStaticMesh(pStaticMesh);

    pStaticMesh->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    pStaticMesh->NeverStream = true;

    pStaticMesh->SetRenderData(std::move(loadResult.RenderData));
  }

  const Material& material =
      loadResult.pMaterial ? *loadResult.pMaterial : defaultMaterial;

  const MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  const FName ImportedSlotName(
      *(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));

  const auto is_in_blend_mode = [](auto& result) {
    return !!result.pMaterial && result.pMaterial->alphaMode ==
                                     CesiumGltf::Material::AlphaMode::BLEND;
  };

#if PLATFORM_MAC
  // TODO: figure out why water material crashes mac
  UMaterialInterface* pBaseMaterial =
      (is_in_blend_mode(loadResult) && pbr.baseColorFactor.size() > 3 &&
       pbr.baseColorFactor[3] < 0.996) // 1. - 1. / 256.
          ? pGltf->BaseMaterialWithTranslucency
          : pGltf->BaseMaterial;
#else
  UMaterialInterface* pBaseMaterial;
  if (loadResult.onlyWater || !loadResult.onlyLand) {
    pBaseMaterial = pGltf->BaseMaterialWithWater;
  } else {
    pBaseMaterial =
        (is_in_blend_mode(loadResult) && pbr.baseColorFactor.size() > 3 &&
         pbr.baseColorFactor[3] < 0.996) // 1. - 1. / 256.
            ? pGltf->BaseMaterialWithTranslucency
            : pGltf->BaseMaterial;
  }
#endif

  UMaterialInstanceDynamic* pMaterial;
  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetupMaterial)

    pMaterial = UMaterialInstanceDynamic::Create(
        pBaseMaterial,
        nullptr,
        ImportedSlotName);

    pMaterial->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    SetGltfParameterValues(
        model,
        loadResult,
        material,
        pbr,
        pMaterial,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);
    SetWaterParameterValues(
        model,
        loadResult,
        pMaterial,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);

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

    if (pCesiumData) {
      SetGltfParameterValues(
          model,
          loadResult,
          material,
          pbr,
          pMaterial,
          EMaterialParameterAssociation::LayerParameter,
          0);

      int fadeLayerIndex = pCesiumData->LayerNames.Find("DitherFade");
      if (fadeLayerIndex >= 0) {
        pMaterial->SetScalarParameterValueByInfo(
            FMaterialParameterInfo(
                "EnableLodTransitions",
                EMaterialParameterAssociation::LayerParameter,
                fadeLayerIndex),
            pTilesetActor->GetUseLodTransitions() ? 1.0f : 0.0f);

        // Initialize fade uniform to fully visible, in case LOD transitions
        // are off.
        pMaterial->SetScalarParameterValueByInfo(
            FMaterialParameterInfo(
                "FadePercentage",
                EMaterialParameterAssociation::LayerParameter,
                fadeLayerIndex),
            1.0f);
        pMaterial->SetScalarParameterValueByInfo(
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
            pMaterial,
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
            pMaterial,
            EMaterialParameterAssociation::LayerParameter,
            featuresMetadataIndex);
      } else if (metadataIndex >= 0) {
        // Set parameters for materials generated by the old implementation
        SetMetadataParameterValues_DEPRECATED(
            model,
            *pGltf,
            loadResult,
            pMaterial,
            EMaterialParameterAssociation::LayerParameter,
            metadataIndex);
      }
    }
  }

  pMesh->Features = std::move(loadResult.Features);
  pMesh->Metadata = std::move(loadResult.Metadata);

  pMesh->EncodedFeatures = std::move(loadResult.EncodedFeatures);
  pMesh->EncodedMetadata = std::move(loadResult.EncodedMetadata);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS

  // Doing the above std::move operations invalidates the pointers in the
  // FCesiumMetadataPrimitive constructed on the loadResult. It's a bit
  // awkward, but we have to reconstruct the metadata primitive here.
  pMesh->Metadata_DEPRECATED = FCesiumMetadataPrimitive{
      pMesh->Features,
      pMesh->Metadata,
      pGltf->Metadata};

  if (loadResult.EncodedMetadata_DEPRECATED) {
    pMesh->EncodedMetadata_DEPRECATED =
        std::move(loadResult.EncodedMetadata_DEPRECATED);
  }

  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  pMaterial->TwoSided = true;

  pStaticMesh->AddMaterial(pMaterial);

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
#if ENGINE_VERSION_5_4_OR_HIGHER
      pBodySetup->TriMeshGeometries.Add(loadResult.pCollisionMesh);
#else
      pBodySetup->ChaosTriMeshes.Add(loadResult.pCollisionMesh);
#endif
    }

    // Mark physics meshes created, no matter if we actually have a collision
    // mesh or not. We don't want the editor creating collision meshes itself in
    // the game thread, because that would be slow.
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

/*static*/ TUniquePtr<UCesiumGltfComponent::HalfConstructed>
UCesiumGltfComponent::CreateOffGameThread(
    const glm::dmat4x4& Transform,
    const CreateModelOptions& Options) {
  std::vector<FCesiumTextureResourceBase*> textureResources;
  textureResources.resize(Options.pModel->images.size(), nullptr);

  auto pResult = MakeUnique<HalfConstructedReal>();
  loadModelAnyThreadPart(
      pResult->loadModelResult,
      Transform,
      Options,
      textureResources);

  return pResult;
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

  for (LoadNodeResult& node : pReal->loadModelResult.nodeResults) {
    if (node.meshResult) {
      for (LoadPrimitiveResult& primitive : node.meshResult->primitiveResults) {
        loadPrimitiveGameThreadPart(
            model,
            Gltf,
            primitive,
            cesiumToUnrealTransform,
            tile,
            createNavCollision,
            pTilesetActor);
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

UCesiumGltfComponent::~UCesiumGltfComponent() {
  UE_LOG(LogCesium, VeryVerbose, TEXT("~UCesiumGltfComponent"));
}

void UCesiumGltfComponent::UpdateTransformFromCesium(
    const glm::dmat4& cesiumToUnrealTransform) {
  for (USceneComponent* pSceneComponent : this->GetAttachChildren()) {
    UCesiumGltfPrimitiveComponent* pPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pSceneComponent);
    if (pPrimitive) {
      pPrimitive->UpdateTransformFromCesium(cesiumToUnrealTransform);
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
            pMaterial->SetScalarParameterValueByInfo(
                FMaterialParameterInfo(
                    "TextureCoordinateIndex",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                static_cast<float>(
                    pPrimitive->overlayTextureCoordinateIDToUVIndex
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
              static_cast<float>(pPrimitive->overlayTextureCoordinateIDToUVIndex
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
  this->EncodedMetadata = CesiumEncodedFeaturesMetadata::EncodedModelMetadata();

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

static bool isTriangleDegenerate(
    const Chaos::FTriangleMeshImplicitObject::ParticleVecType& A,
    const Chaos::FTriangleMeshImplicitObject::ParticleVecType& B,
    const Chaos::FTriangleMeshImplicitObject::ParticleVecType& C) {
  Chaos::FTriangleMeshImplicitObject::ParticleVecType AB = B - A;
  Chaos::FTriangleMeshImplicitObject::ParticleVecType AC = C - A;
  Chaos::FTriangleMeshImplicitObject::ParticleVecType Normal =
      Chaos::FTriangleMeshImplicitObject::ParticleVecType::CrossProduct(AB, AC);
  return (Normal.SafeNormalize() < 1.e-8f);
}

template <typename TIndex>
#if ENGINE_VERSION_5_4_OR_HIGHER
static Chaos::FTriangleMeshImplicitObjectPtr
#else
static TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
#endif
BuildChaosTriangleMeshes(
    const TArray<FStaticMeshBuildVertex>& vertexData,
    const TArray<uint32>& indices) {
  int32 vertexCount = vertexData.Num();
  Chaos::TParticles<Chaos::FRealSingle, 3> vertices;
  vertices.AddParticles(vertexCount);
  for (int32 i = 0; i < vertexCount; ++i) {
    vertices.X(i) = vertexData[i].Position;
  }

  int32 triangleCount = indices.Num() / 3;
  TArray<Chaos::TVector<TIndex, 3>> triangles;
  triangles.Reserve(triangleCount);
  TArray<int32> faceRemap;
  faceRemap.Reserve(triangleCount);

  for (int32 i = 0; i < triangleCount; ++i) {
    const int32 index0 = 3 * i;
    int32 vIndex0 = indices[index0 + 1];
    int32 vIndex1 = indices[index0];
    int32 vIndex2 = indices[index0 + 2];

    if (!isTriangleDegenerate(
            vertices.X(vIndex0),
            vertices.X(vIndex1),
            vertices.X(vIndex2))) {
      triangles.Add(Chaos::TVector<int32, 3>(vIndex0, vIndex1, vIndex2));
      faceRemap.Add(i);
    }
  }

  TUniquePtr<TArray<int32>> pFaceRemap = MakeUnique<TArray<int32>>(faceRemap);
  TArray<uint16> materials;
  materials.SetNum(triangles.Num());

#if ENGINE_VERSION_5_4_OR_HIGHER
  return new Chaos::FTriangleMeshImplicitObject(
      MoveTemp(vertices),
      MoveTemp(triangles),
      MoveTemp(materials),
      MoveTemp(pFaceRemap),
      nullptr,
      false);
#else
  return MakeShared<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>(
      MoveTemp(vertices),
      MoveTemp(triangles),
      MoveTemp(materials),
      MoveTemp(pFaceRemap),
      nullptr,
      false);
#endif
}
