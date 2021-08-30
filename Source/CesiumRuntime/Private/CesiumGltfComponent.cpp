// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfComponent.h"
#include "Async/Async.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "CesiumGeometry/Axis.h"
#include "CesiumGeometry/AxisTransforms.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/GltfReader.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/TextureInfo.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMaterialUserData.h"
#include "CesiumRasterOverlays.h"
#include "CesiumRuntime.h"
#include "CesiumTextureUtility.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Tracing.h"
#include "CesiumUtility/joinToString.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshTypes.h"
#include "PhysicsEngine/BodySetup.h"
#include "PixelFormat.h"
#include "StaticMeshOperations.h"
#include "StaticMeshResources.h"
#include "UObject/ConstructorHelpers.h"
#include "mikktspace.h"
#include <cstddef>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <iostream>

#if PHYSICS_INTERFACE_PHYSX
#include "IPhysXCooking.h"
#else
#include "Chaos/CollisionConvexMesh.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "ChaosDerivedDataUtil.h"
#endif

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

using namespace CesiumGltf;

static uint32_t nextMaterialId = 0;

struct CustomMask {
  std::string name;
  CesiumTextureUtility::LoadedTextureResult* loadTextureResult;
};

struct LoadModelResult {
  FCesiumMetadataPrimitive Metadata;
  FStaticMeshRenderData* RenderData;
  const CesiumGltf::Model* pModel;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive;
  const CesiumGltf::Material* pMaterial;
  glm::dmat4x4 transform;
#if PHYSICS_INTERFACE_PHYSX
  PxTriangleMesh* pCollisionMesh;
#else
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
      pCollisionMesh;
#endif
  std::string name;

  CesiumTextureUtility::LoadedTextureResult* baseColorTexture;
  CesiumTextureUtility::LoadedTextureResult* metallicRoughnessTexture;
  CesiumTextureUtility::LoadedTextureResult* normalTexture;
  CesiumTextureUtility::LoadedTextureResult* emissiveTexture;
  CesiumTextureUtility::LoadedTextureResult* occlusionTexture;
  CesiumTextureUtility::LoadedTextureResult* waterMaskTexture;
  std::vector<CustomMask> customMaskTextures;
  std::unordered_map<std::string, uint32_t> textureCoordinateParameters;

  bool onlyLand;
  bool onlyWater;

  double waterMaskTranslationX;
  double waterMaskTranslationY;
  double waterMaskScale;

  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex;
};

template <class... T> struct IsAccessorView;

template <class T> struct IsAccessorView<T> : std::false_type {};

template <class T>
struct IsAccessorView<CesiumGltf::AccessorView<T>> : std::true_type {};

template <class T>
static uint32_t updateTextureCoordinates(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    bool duplicateVertices,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices,
    const std::optional<T>& texture,
    std::unordered_map<uint32_t, uint32_t>& textureCoordinateMap) {
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
      textureCoordinateMap);
}

uint32_t updateTextureCoordinates(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    bool duplicateVertices,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices,
    const std::string& attributeName,
    std::unordered_map<uint32_t, uint32_t>& textureCoordinateMap) {
  auto uvAccessorIt = primitive.attributes.find(attributeName);
  if (uvAccessorIt == primitive.attributes.end()) {
    // Texture not used, texture coordinates don't matter.
    return 0;
  }

  int uvAccessorID = uvAccessorIt->second;
  auto mapIt = textureCoordinateMap.find(uvAccessorID);
  if (mapIt != textureCoordinateMap.end()) {
    // Texture coordinates for this accessor are already populated.
    return mapIt->second;
  }

  size_t textureCoordinateIndex = textureCoordinateMap.size();
  textureCoordinateMap[uvAccessorID] = textureCoordinateIndex;

  CesiumGltf::AccessorView<FVector2D> uvAccessor(model, uvAccessorID);
  if (uvAccessor.status() != CesiumGltf::AccessorViewStatus::Valid) {
    return 0;
  }

  if (duplicateVertices) {
    for (int64_t i = 0; i < indices.Num(); ++i) {
      FStaticMeshBuildVertex& vertex = vertices[i];
      uint32 vertexIndex = indices[i];
      if (vertexIndex >= 0 && vertexIndex < uvAccessor.size()) {
        vertex.UVs[textureCoordinateIndex] = uvAccessor[vertexIndex];
      } else {
        vertex.UVs[textureCoordinateIndex] = FVector2D(0.0f, 0.0f);
      }
    }
  } else {
    for (int64_t i = 0; i < vertices.Num(); ++i) {
      FStaticMeshBuildVertex& vertex = vertices[i];
      if (i >= 0 && i < uvAccessor.size()) {
        vertex.UVs[textureCoordinateIndex] = uvAccessor[i];
      } else {
        vertex.UVs[textureCoordinateIndex] = FVector2D(0.0f, 0.0f);
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
  FVector& position = vertices[FaceIdx * 3 + VertIdx].Position;
  Position[0] = position.X;
  Position[1] = position.Y;
  Position[2] = position.Z;
}

static void mikkGetNormal(
    const SMikkTSpaceContext* Context,
    float Normal[3],
    const int FaceIdx,
    const int VertIdx) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  FVector& normal = vertices[FaceIdx * 3 + VertIdx].TangentZ;
  Normal[0] = normal.X;
  Normal[1] = normal.Y;
  Normal[2] = normal.Z;
}

static void mikkGetTexCoord(
    const SMikkTSpaceContext* Context,
    float UV[2],
    const int FaceIdx,
    const int VertIdx) {
  TArray<FStaticMeshBuildVertex>& vertices =
      *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
  FVector2D& uv = vertices[FaceIdx * 3 + VertIdx].UVs[0];
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
  vertex.TangentX = FVector(Tangent[0], Tangent[1], Tangent[2]);
  vertex.TangentY =
      BitangentSign * FVector::CrossProduct(vertex.TangentZ, vertex.TangentX);
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

static void computeFlatNormals(
    const TArray<uint32_t>& indices,
    TArray<FStaticMeshBuildVertex>& vertices) {
  // Compute flat normals
  for (int64_t i = 0; i < indices.Num(); i += 3) {
    FStaticMeshBuildVertex& v0 = vertices[i];
    FStaticMeshBuildVertex& v1 = vertices[i + 1];
    FStaticMeshBuildVertex& v2 = vertices[i + 2];

    FVector v01 = v1.Position - v0.Position;
    FVector v02 = v2.Position - v0.Position;
    FVector normal = FVector::CrossProduct(v01, v02);

    v0.TangentX = v1.TangentX = v2.TangentX = FVector(0.0f);
    v0.TangentY = v1.TangentY = v2.TangentY = FVector(0.0f);
    v0.TangentZ = v1.TangentZ = v2.TangentZ = normal.GetSafeNormal();
  }
}

#if !PHYSICS_INTERFACE_PHYSX
static TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
BuildChaosTriangleMeshes(
    const TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices);
#endif

static const CesiumGltf::Material defaultMaterial;
static const CesiumGltf::MaterialPBRMetallicRoughness
    defaultPbrMetallicRoughness;

struct ColorVisitor {
  bool duplicateVertices;
  TArray<FStaticMeshBuildVertex>& StaticMeshBuildVertices;
  const TArray<uint32>& indices;

  bool operator()(AccessorView<nullptr_t>&& invalidView) { return false; }

  template <typename TColorView> bool operator()(TColorView&& colorView) {
    if (colorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      return false;
    }

    bool success = true;
    if (duplicateVertices) {
      for (int64_t i = 0; success && i < this->indices.Num(); ++i) {
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
      for (int64_t i = 0; success && i < this->StaticMeshBuildVertices.Num();
           ++i) {
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
static CesiumTextureUtility::LoadedTextureResult* loadTexture(
    const CesiumGltf::Model& model,
    const std::optional<T>& gltfTexture) {
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

  const CesiumGltf::Texture& texture =
      model.textures[gltfTexture.value().index];

  return CesiumTextureUtility::loadTextureAnyThreadPart(model, texture);
}

static void applyWaterMask(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    LoadModelResult& primitiveResult) {
  // Initialize water mask if needed.
  auto onlyWaterIt = primitive.extras.find("OnlyWater");
  auto onlyLandIt = primitive.extras.find("OnlyLand");
  if (onlyWaterIt != primitive.extras.end() && onlyWaterIt->second.isBool() &&
      onlyLandIt != primitive.extras.end() && onlyLandIt->second.isBool()) {
    CESIUM_TRACE("water mask");
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
              loadTexture(model, std::make_optional(waterMaskInfo));
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

static FCesiumMetadataPrimitive loadMetadataPrimitive(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive) {
  const CesiumGltf::ModelEXT_feature_metadata* metadata =
      model.getExtension<CesiumGltf::ModelEXT_feature_metadata>();
  if (!metadata) {
    return FCesiumMetadataPrimitive();
  }

  const CesiumGltf::MeshPrimitiveEXT_feature_metadata* primitiveMetadata =
      primitive.getExtension<CesiumGltf::MeshPrimitiveEXT_feature_metadata>();
  if (!primitiveMetadata) {
    return FCesiumMetadataPrimitive();
  }

  return FCesiumMetadataPrimitive(
      model,
      primitive,
      *metadata,
      *primitiveMetadata);
}

template <class TIndexAccessor>
static void loadPrimitive(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::dmat4x4& transform,
    const CreateModelOptions& options,
    const CesiumGltf::Accessor& positionAccessor,
    const CesiumGltf::AccessorView<FVector>& positionView,
    const TIndexAccessor& indicesView) {

  CESIUM_TRACE("loadPrimitive<T>");

  if (primitive.mode != CesiumGltf::MeshPrimitive::Mode::TRIANGLES &&
      primitive.mode != CesiumGltf::MeshPrimitive::Mode::TRIANGLE_STRIP) {
    // TODO: add support for primitive types other than triangles.
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Primitive mode %d is not supported"),
        primitive.mode);
    return;
  }

  LoadModelResult primitiveResult;

  std::string name = "glTF";

  auto urlIt = model.extras.find("Cesium3DTiles_TileUrl");
  if (urlIt != model.extras.end()) {
    name = urlIt->second.getStringOrDefault("glTF");
  }

  auto meshIt = std::find_if(
      model.meshes.begin(),
      model.meshes.end(),
      [&mesh](const CesiumGltf::Mesh& candidate) {
        return &candidate == &mesh;
      });
  if (meshIt != model.meshes.end()) {
    int64_t meshIndex = meshIt - model.meshes.begin();
    name += " mesh " + std::to_string(meshIndex);
  }

  auto primitiveIt = std::find_if(
      mesh.primitives.begin(),
      mesh.primitives.end(),
      [&primitive](const CesiumGltf::MeshPrimitive& candidate) {
        return &candidate == &primitive;
      });
  if (primitiveIt != mesh.primitives.end()) {
    int64_t primitiveIndex = primitiveIt - mesh.primitives.begin();
    name += " primitive " + std::to_string(primitiveIndex);
  }

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

  auto normalAccessorIt = primitive.attributes.find("NORMAL");
  CesiumGltf::AccessorView<FVector> normalAccessor;
  bool hasNormals = false;
  if (normalAccessorIt != primitive.attributes.end()) {
    int normalAccessorID = normalAccessorIt->second;
    normalAccessor = CesiumGltf::AccessorView<FVector>(model, normalAccessorID);
    hasNormals =
        normalAccessor.status() == CesiumGltf::AccessorViewStatus::Valid;
    if (!hasNormals) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "%s: Invalid normal buffer. Flat normal will be auto-generated instead"),
          UTF8_TO_TCHAR(name.c_str()));
    }
  }

  int materialID = primitive.material;
  const CesiumGltf::Material& material =
      materialID >= 0 && materialID < model.materials.size()
          ? model.materials[materialID]
          : defaultMaterial;
  const CesiumGltf::MaterialPBRMetallicRoughness& pbrMetallicRoughness =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  bool hasNormalMap = material.normalTexture.has_value();
  if (hasNormalMap) {
    const CesiumGltf::Texture* pTexture =
        Model::getSafe(&model.textures, material.normalTexture->index);
    hasNormalMap = pTexture != nullptr &&
                   Model::getSafe(&model.images, pTexture->source) != nullptr;
  }

  bool needsTangents = hasNormalMap || options.alwaysIncludeTangents;

  bool hasTangents = false;
  auto tangentAccessorIt = primitive.attributes.find("TANGENT");
  CesiumGltf::AccessorView<FVector4> tangentAccessor;
  if (tangentAccessorIt != primitive.attributes.end()) {
    int tangentAccessorID = tangentAccessorIt->second;
    tangentAccessor =
        CesiumGltf::AccessorView<FVector4>(model, tangentAccessorID);
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

  FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();
  RenderData->AllocateLODResources(1);

  FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

  {
    CESIUM_TRACE("compute AA bounding box");

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

    FBox aaBox(
        FVector(minPosition.x, minPosition.y, minPosition.z),
        FVector(maxPosition.x, maxPosition.y, maxPosition.z));

    aaBox.GetCenterAndExtents(
        RenderData->Bounds.Origin,
        RenderData->Bounds.BoxExtent);
    RenderData->Bounds.SphereRadius = 0.0f;
  }

  TArray<uint32> indices;
  if (primitive.mode == CesiumGltf::MeshPrimitive::Mode::TRIANGLES) {
    CESIUM_TRACE("copy TRIANGLE indices");
    indices.SetNum(static_cast<TArray<uint32>::SizeType>(indicesView.size()));

    for (int32 i = 0; i < indicesView.size(); ++i) {
      indices[i] = indicesView[i];
    }
  } else {
    // assume TRIANGLE_STRIP because all others are rejected earlier.
    CESIUM_TRACE("copy TRIANGLE_STRIP indices");
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

  TArray<FStaticMeshBuildVertex> StaticMeshBuildVertices;
  StaticMeshBuildVertices.SetNum(
      duplicateVertices ? indices.Num() : positionView.size());

  // The static mesh we construct will _not_ be indexed, even if the incoming
  // glTF is. This allows us to compute flat normals if the glTF doesn't include
  // them already, and it allows us to compute a correct tangent space basis
  // according to the MikkTSpace algorithm when tangents are not included in the
  // glTF.

  {
    if (duplicateVertices) {
      CESIUM_TRACE("copy duplicated positions");
      for (int64_t i = 0; i < indices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        uint32 vertexIndex = indices[i];
        vertex.Position = positionView[vertexIndex];
        vertex.UVs[0] = FVector2D(0.0f, 0.0f);
        vertex.UVs[2] = FVector2D(0.0f, 0.0f);
        RenderData->Bounds.SphereRadius = FMath::Max(
            (vertex.Position - RenderData->Bounds.Origin).Size(),
            RenderData->Bounds.SphereRadius);
      }
    } else {
      CESIUM_TRACE("copy positions");
      for (int64_t i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        vertex.Position = positionView[i];
        vertex.UVs[0] = FVector2D(0.0f, 0.0f);
        vertex.UVs[2] = FVector2D(0.0f, 0.0f);
        RenderData->Bounds.SphereRadius = FMath::Max(
            (vertex.Position - RenderData->Bounds.Origin).Size(),
            RenderData->Bounds.SphereRadius);
      }
    }
  }

  // TangentX: Tangent
  // TangentY: Bi-tangent
  // TangentZ: Normal

  if (hasNormals) {
    if (duplicateVertices) {
      CESIUM_TRACE("copy normals for duplicated vertices");
      for (int64_t i = 0; i < indices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        uint32 vertexIndex = indices[i];
        vertex.TangentX = FVector(0.0f, 0.0f, 0.0f);
        vertex.TangentY = FVector(0.0f, 0.0f, 0.0f);
        vertex.TangentZ = normalAccessor[vertexIndex];
      }
    } else {
      CESIUM_TRACE("copy normals");
      for (int64_t i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        vertex.TangentX = FVector(0.0f, 0.0f, 0.0f);
        vertex.TangentY = FVector(0.0f, 0.0f, 0.0f);
        vertex.TangentZ = normalAccessor[i];
      }
    }
  } else {
    CESIUM_TRACE("compute flat normals");
    computeFlatNormals(indices, StaticMeshBuildVertices);
  }

  if (hasTangents) {
    if (duplicateVertices) {
      CESIUM_TRACE("copy tangents for duplicated vertices");
      for (int64_t i = 0; i < indices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        uint32 vertexIndex = indices[i];
        const FVector4& tangent = tangentAccessor[vertexIndex];
        vertex.TangentX = tangent;
        vertex.TangentY =
            FVector::CrossProduct(vertex.TangentZ, vertex.TangentX) * tangent.W;
      }
    } else {
      CESIUM_TRACE("copy tangents");
      for (int64_t i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
        FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
        const FVector4& tangent = tangentAccessor[i];
        vertex.TangentX = tangent;
        vertex.TangentY =
            FVector::CrossProduct(vertex.TangentZ, vertex.TangentX) * tangent.W;
      }
    }
  }

  if (needsTangents && !hasTangents) {
    // Use mikktspace to calculate the tangents
    CESIUM_TRACE("compute tangents");
    computeTangentSpace(StaticMeshBuildVertices);
  }

  bool hasVertexColors = false;

  auto colorAccessorIt = primitive.attributes.find("COLOR_0");
  if (colorAccessorIt != primitive.attributes.end()) {
    CESIUM_TRACE("copy colors");
    int colorAccessorID = colorAccessorIt->second;
    hasVertexColors = CesiumGltf::createAccessorView(
        model,
        colorAccessorID,
        ColorVisitor{duplicateVertices, StaticMeshBuildVertices, indices});
  }

  LODResources.bHasColorVertexData = hasVertexColors;

  // We need to copy the texture coordinates associated with each texture (if
  // any) into the the appropriate UVs slot in FStaticMeshBuildVertex.

  std::unordered_map<uint32_t, uint32_t> textureCoordinateMap;

  {
    CESIUM_TRACE("loadTextures");
    primitiveResult.baseColorTexture =
        loadTexture(model, pbrMetallicRoughness.baseColorTexture);
    primitiveResult.metallicRoughnessTexture =
        loadTexture(model, pbrMetallicRoughness.metallicRoughnessTexture);
    primitiveResult.normalTexture = loadTexture(model, material.normalTexture);
    primitiveResult.occlusionTexture =
        loadTexture(model, material.occlusionTexture);
    primitiveResult.emissiveTexture =
        loadTexture(model, material.emissiveTexture);
  }

  {
    CESIUM_TRACE("updateTextureCoordinates");
    primitiveResult
        .textureCoordinateParameters["baseColorTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            pbrMetallicRoughness.baseColorTexture,
            textureCoordinateMap);
    primitiveResult.textureCoordinateParameters
        ["metallicRoughnessTextureCoordinateIndex"] = updateTextureCoordinates(
        model,
        primitive,
        duplicateVertices,
        StaticMeshBuildVertices,
        indices,
        pbrMetallicRoughness.metallicRoughnessTexture,
        textureCoordinateMap);
    primitiveResult
        .textureCoordinateParameters["normalTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            material.normalTexture,
            textureCoordinateMap);
    primitiveResult
        .textureCoordinateParameters["occlusionTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            material.occlusionTexture,
            textureCoordinateMap);
    primitiveResult
        .textureCoordinateParameters["emissiveTextureCoordinateIndex"] =
        updateTextureCoordinates(
            model,
            primitive,
            duplicateVertices,
            StaticMeshBuildVertices,
            indices,
            material.emissiveTexture,
            textureCoordinateMap);

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
                textureCoordinateMap);
      } else {
        primitiveResult.overlayTextureCoordinateIDToUVIndex[i] = 0;
      }
    }
  }

  {
    CESIUM_TRACE("init buffers");
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
        textureCoordinateMap.size() == 0 ? 1 : textureCoordinateMap.size(),
        false);
  }

  FStaticMeshLODResources::FStaticMeshSectionArray& Sections =
      LODResources.Sections;
  FStaticMeshSection& section = Sections.AddDefaulted_GetRef();
  section.bEnableCollision = true;

  section.NumTriangles = indices.Num() / 3;
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = StaticMeshBuildVertices.Num() - 1;
  section.bEnableCollision = true;
  section.bCastShadow = true;

  // Note that we're reversing the order of the indices, because the change
  // from the glTF right-handed to the Unreal left-handed coordinate system
  // reverses the winding order.
  // Note also that we don't want to just flip the index buffer, since that
  // will change the order of the faces.
  if (duplicateVertices) {
    CESIUM_TRACE("reverse winding order of duplicated vertices");
    for (int32 i = 2; i < indices.Num(); i += 3) {
      indices[i - 2] = i;
      indices[i - 1] = i - 1;
      indices[i] = i - 2;
    }
  } else {
    CESIUM_TRACE("reverse winding order");
    for (int32 i = 2; i < indices.Num(); i += 3) {
      std::swap(indices[i - 2], indices[i]);
    }
  }

  {
    CESIUM_TRACE("SetIndices");
    LODResources.IndexBuffer.SetIndices(
        indices,
        indices.Num() > std::numeric_limits<uint16>::max()
            ? EIndexBufferStride::Type::Force32Bit
            : EIndexBufferStride::Type::Force16Bit);
  }

  LODResources.bHasDepthOnlyIndices = false;
  LODResources.bHasReversedIndices = false;
  LODResources.bHasReversedDepthOnlyIndices = false;
  LODResources.bHasAdjacencyInfo = false;

  primitiveResult.pModel = &model;
  primitiveResult.pMeshPrimitive = &primitive;
  primitiveResult.RenderData = RenderData;
  primitiveResult.transform = transform;
  primitiveResult.pMaterial = &material;

  section.MaterialIndex = 0;

  primitiveResult.pCollisionMesh = nullptr;

#if PHYSICS_INTERFACE_PHYSX
  if (options.pPhysXCooking) {
    CESIUM_TRACE("PhysX cook");
    // TODO: use PhysX interface directly so we don't need to copy the
    // vertices (it takes a stride parameter).
    TArray<FVector> vertices;
    vertices.SetNum(StaticMeshBuildVertices.Num());

    for (size_t i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
      vertices[i] = StaticMeshBuildVertices[i].Position;
    }

    TArray<FTriIndices> physicsIndices;
    physicsIndices.SetNum(indices.Num() / 3);

    for (size_t i = 0; i < indices.Num() / 3; ++i) {
      physicsIndices[i].v0 = indices[3 * i];
      physicsIndices[i].v1 = indices[3 * i + 1];
      physicsIndices[i].v2 = indices[3 * i + 2];
    }

    options.pPhysXCooking->CreateTriMesh(
        "PhysXGeneric",
        EPhysXMeshCookFlags::Default,
        vertices,
        physicsIndices,
        TArray<uint16>(),
        true,
        primitiveResult.pCollisionMesh);
  }
#else
  if (StaticMeshBuildVertices.Num() != 0 && indices.Num() != 0) {
    CESIUM_TRACE("Chaos cook");
    primitiveResult.pCollisionMesh = BuildChaosTriangleMeshes(
        duplicateVertices,
        StaticMeshBuildVertices,
        indices);
  }
#endif

  // load primitive metadata
  primitiveResult.Metadata = loadMetadataPrimitive(model, primitive);

  result.push_back(std::move(primitiveResult));
}

static void loadIndexedPrimitive(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::dmat4x4& transform,
    const CreateModelOptions& options,
    const CesiumGltf::Accessor& positionAccessor,
    const CesiumGltf::AccessorView<FVector>& positionView) {
  const CesiumGltf::Accessor& indexAccessorGltf =
      model.accessors[primitive.indices];
  if (indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::BYTE) {
    CesiumGltf::AccessorView<int8_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor);
  } else if (
      indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE) {
    CesiumGltf::AccessorView<uint8_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor);
  } else if (
      indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::SHORT) {
    CesiumGltf::AccessorView<int16_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor);
  } else if (
      indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT) {
    CesiumGltf::AccessorView<uint16_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor);
  } else if (
      indexAccessorGltf.componentType ==
      CesiumGltf::Accessor::ComponentType::UNSIGNED_INT) {
    CesiumGltf::AccessorView<uint32_t> indexAccessor(model, primitive.indices);
    loadPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform,
        options,
        positionAccessor,
        positionView,
        indexAccessor);
  }
}

static void loadPrimitive(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::dmat4x4& transform,
    const CreateModelOptions& options) {
  CESIUM_TRACE("loadPrimitive");

  auto positionAccessorIt = primitive.attributes.find("POSITION");
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

  CesiumGltf::AccessorView<FVector> positionView(model, *pPositionAccessor);

  if (primitive.indices < 0 || primitive.indices >= model.accessors.size()) {
    std::vector<uint32_t> syntheticIndexBuffer(positionView.size());
    syntheticIndexBuffer.resize(positionView.size());
    for (uint32_t i = 0; i < positionView.size(); ++i) {
      syntheticIndexBuffer[i] = i;
    }
    loadPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform,
        options,
        *pPositionAccessor,
        positionView,
        syntheticIndexBuffer);
  } else {
    loadIndexedPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform,
        options,
        *pPositionAccessor,
        positionView);
  }
}

static void loadMesh(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const glm::dmat4x4& transform,
    const CreateModelOptions& options) {

  CESIUM_TRACE("loadMesh");

  for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
    loadPrimitive(result, model, mesh, primitive, transform, options);
  }
}

static void loadNode(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Node& node,
    const glm::dmat4x4& transform,
    const CreateModelOptions& options) {
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

  CESIUM_TRACE("loadNode");

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
    const CesiumGltf::Mesh& mesh = model.meshes[meshId];
    loadMesh(result, model, mesh, nodeTransform, options);
  }

  for (int childNodeId : node.children) {
    if (childNodeId >= 0 && childNodeId < model.nodes.size()) {
      loadNode(result, model, model.nodes[childNodeId], nodeTransform, options);
    }
  }
}

namespace {
/**
 * @brief Apply the transform for the `RTC_CENTER`
 *
 * If the B3DM that contained the given model had an `RTC_CENTER` in its
 * Feature Table, then it was stored in the `extras` property of the glTF
 * model, as a 3-element array under the name `RTC_CENTER`.
 *
 * This function will multiply the given matrix with the (translation) matrix
 * that was created from this `RTC_CENTER` property in the `extras` of the
 * given model. If the given model does not have this property, then nothing
 * will be done.
 *
 * @param model The glTF model
 * @param rootTransform The matrix that will be multiplied with the transform
 */
void applyRtcCenter(
    const CesiumGltf::Model& model,
    glm::dmat4x4& rootTransform) {
  auto rtcCenterIt = model.extras.find("RTC_CENTER");
  if (rtcCenterIt == model.extras.end()) {
    return;
  }
  const CesiumUtility::JsonValue& rtcCenter = rtcCenterIt->second;
  const std::vector<CesiumUtility::JsonValue>* pArray =
      std::get_if<CesiumUtility::JsonValue::Array>(&rtcCenter.value);
  if (!pArray) {
    return;
  }
  if (pArray->size() != 3) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("The RTC_CENTER must have a size of 3, but has {}"),
        pArray->size());
    return;
  }
  const double x = (*pArray)[0].getSafeNumberOrDefault(0.0);
  const double y = (*pArray)[1].getSafeNumberOrDefault(0.0);
  const double z = (*pArray)[2].getSafeNumberOrDefault(0.0);
  const glm::dmat4x4 rtcTransform(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(x, y, z, 1.0));
  rootTransform *= rtcTransform;
}

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
    rootTransform *= CesiumGeometry::AxisTransforms::Y_UP_TO_Z_UP;
    return;
  }
  const CesiumUtility::JsonValue& gltfUpAxis = gltfUpAxisIt->second;
  int gltfUpAxisValue = static_cast<int>(gltfUpAxis.getSafeNumberOrDefault(1));
  if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::X)) {
    rootTransform *= CesiumGeometry::AxisTransforms::X_UP_TO_Z_UP;
  } else if (gltfUpAxisValue == static_cast<int>(CesiumGeometry::Axis::Y)) {
    rootTransform *= CesiumGeometry::AxisTransforms::Y_UP_TO_Z_UP;
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

static std::vector<LoadModelResult> loadModelAnyThreadPart(
    const CesiumGltf::Model& model,
    const glm::dmat4x4& transform,
    const CreateModelOptions& options) {
  CESIUM_TRACE("loadModelAnyThreadPart");

  std::vector<LoadModelResult> result;

  glm::dmat4x4 rootTransform = transform;

  {
    CESIUM_TRACE("Apply transforms");
    applyRtcCenter(model, rootTransform);
    applyGltfUpAxisTransform(model, rootTransform);
  }

  if (model.scene >= 0 && model.scene < model.scenes.size()) {
    // Show the default scene
    const CesiumGltf::Scene& defaultScene = model.scenes[model.scene];
    for (int nodeId : defaultScene.nodes) {
      loadNode(result, model, model.nodes[nodeId], rootTransform, options);
    }
  } else if (model.scenes.size() > 0) {
    // There's no default, so show the first scene
    const CesiumGltf::Scene& defaultScene = model.scenes[0];
    for (int nodeId : defaultScene.nodes) {
      loadNode(result, model, model.nodes[nodeId], rootTransform, options);
    }
  } else if (model.nodes.size() > 0) {
    // No scenes at all, use the first node as the root node.
    loadNode(result, model, model.nodes[0], rootTransform, options);
  } else if (model.meshes.size() > 0) {
    // No nodes either, show all the meshes.
    for (const CesiumGltf::Mesh& mesh : model.meshes) {
      loadMesh(result, model, mesh, rootTransform, options);
    }
  }

  return result;
}

bool applyTexture(
    UMaterialInstanceDynamic* pMaterial,
    const FMaterialParameterInfo& info,
    CesiumTextureUtility::LoadedTextureResult* pLoadedTexture) {

  if (!CesiumTextureUtility::loadTextureGameThreadPart(pLoadedTexture)) {
    return false;
  }

  UTexture2D* pTexture = pLoadedTexture->pTexture;
  if (!pTexture) {
    return false;
  }

  pMaterial->SetTextureParameterValueByInfo(info, pTexture);

  return true;
}

static void SetGltfParameterValues(
    LoadModelResult& loadResult,
    const CesiumGltf::Material& material,
    const CesiumGltf::MaterialPBRMetallicRoughness& pbr,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation assocation,
    int32 index) {
  for (auto& textureCoordinateSet : loadResult.textureCoordinateParameters) {
    pMaterial->SetScalarParameterValueByInfo(
        FMaterialParameterInfo(
            textureCoordinateSet.first.c_str(),
            assocation,
            index),
        textureCoordinateSet.second);
  }

  if (pbr.baseColorFactor.size() >= 3) {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("baseColorFactor", assocation, index),
        FVector(
            pbr.baseColorFactor[0],
            pbr.baseColorFactor[1],
            pbr.baseColorFactor[2]));
  }
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("metallicFactor", assocation, index),
      pbr.metallicFactor);
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("roughnessFactor", assocation, index),
      pbr.roughnessFactor);
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("opacityMask", assocation, index),
      1.0);

  applyTexture(
      pMaterial,
      FMaterialParameterInfo("baseColorTexture", assocation, index),
      loadResult.baseColorTexture);
  applyTexture(
      pMaterial,
      FMaterialParameterInfo("metallicRoughnessTexture", assocation, index),
      loadResult.metallicRoughnessTexture);
  applyTexture(
      pMaterial,
      FMaterialParameterInfo("normalTexture", assocation, index),
      loadResult.normalTexture);
  bool hasEmissiveTexture = applyTexture(
      pMaterial,
      FMaterialParameterInfo("emissiveTexture", assocation, index),
      loadResult.emissiveTexture);
  applyTexture(
      pMaterial,
      FMaterialParameterInfo("occlusionTexture", assocation, index),
      loadResult.occlusionTexture);

  if (material.emissiveFactor.size() >= 3) {
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("emissiveFactor", assocation, index),
        FVector(
            material.emissiveFactor[0],
            material.emissiveFactor[1],
            material.emissiveFactor[2]));
  } else if (hasEmissiveTexture) {
    // When we have an emissive texture but not a factor, we need to use a
    // factor of vec3(1.0). The default, vec3(0.0), would disable the emission
    // from the texture.
    pMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo("emissiveFactor", assocation, index),
        FVector(1.0f, 1.0f, 1.0f));
  }
}

void SetWaterParameterValues(
    LoadModelResult& loadResult,
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation assocation,
    int32 index) {
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("OnlyLand", assocation, index),
      static_cast<float>(loadResult.onlyLand));
  pMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo("OnlyWater", assocation, index),
      static_cast<float>(loadResult.onlyWater));

  if (!loadResult.onlyLand && !loadResult.onlyWater) {
    applyTexture(
        pMaterial,
        FMaterialParameterInfo("WaterMask", assocation, index),
        loadResult.waterMaskTexture);
  }

  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo("WaterMaskTranslationScale", assocation, index),
      FLinearColor(
          loadResult.waterMaskTranslationX,
          loadResult.waterMaskTranslationY,
          loadResult.waterMaskScale));
}

static void loadModelGameThreadPart(
    UCesiumGltfComponent* pGltf,
    LoadModelResult& loadResult,
    const glm::dmat4x4& cesiumToUnrealTransform) {
  UCesiumGltfPrimitiveComponent* pMesh =
      NewObject<UCesiumGltfPrimitiveComponent>(
          pGltf,
          FName(loadResult.name.c_str()));
  pMesh->overlayTextureCoordinateIDToUVIndex =
      loadResult.overlayTextureCoordinateIDToUVIndex;
  pMesh->HighPrecisionNodeTransform = loadResult.transform;
  pMesh->UpdateTransformFromCesium(cesiumToUnrealTransform);

  pMesh->bUseDefaultCollision = false;
  pMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
  pMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pMesh->Metadata = std::move(loadResult.Metadata);
  pMesh->pModel = loadResult.pModel;
  pMesh->pMeshPrimitive = loadResult.pMeshPrimitive;

  UStaticMesh* pStaticMesh =
      NewObject<UStaticMesh>(pMesh, FName(loadResult.name.c_str()));
  pMesh->SetStaticMesh(pStaticMesh);

  pStaticMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pStaticMesh->NeverStream = true;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
  pStaticMesh->bIsBuiltAtRuntime = true;
  pStaticMesh->RenderData =
      TUniquePtr<FStaticMeshRenderData>(loadResult.RenderData);
#else
  pStaticMesh->SetIsBuiltAtRuntime(true);
  pStaticMesh->SetRenderData(
      TUniquePtr<FStaticMeshRenderData>(loadResult.RenderData));
#endif

  const CesiumGltf::Model& model = *loadResult.pModel;
  const CesiumGltf::Material& material =
      loadResult.pMaterial ? *loadResult.pMaterial : defaultMaterial;

  const CesiumGltf::MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  const FName ImportedSlotName(
      *(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));

#if PLATFORM_MAC
  // TODO: figure out why water material crashes mac
  UMaterialInterface* pBaseMaterial = pGltf->BaseMaterial;
#else
  UMaterialInterface* pBaseMaterial =
      (loadResult.onlyWater || !loadResult.onlyLand)
          ? pGltf->BaseMaterialWithWater
          : pGltf->BaseMaterial;
#endif

  UMaterialInstanceDynamic* pMaterial = UMaterialInstanceDynamic::Create(
      pBaseMaterial,
      nullptr,
      ImportedSlotName);

  pMaterial->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  SetGltfParameterValues(
      loadResult,
      material,
      pbr,
      pMaterial,
      EMaterialParameterAssociation::GlobalParameter,
      INDEX_NONE);
  SetWaterParameterValues(
      loadResult,
      pMaterial,
      EMaterialParameterAssociation::GlobalParameter,
      INDEX_NONE);

  UMaterialInstance* pBaseAsMaterialInstance =
      Cast<UMaterialInstance>(pBaseMaterial);
  UCesiumMaterialUserData* pCesiumData =
      pBaseAsMaterialInstance
          ? pBaseAsMaterialInstance->GetAssetUserData<UCesiumMaterialUserData>()
          : nullptr;

  // If possible and necessary, attach the CesiumMaterialUserData now.
#if WITH_EDITORONLY_DATA
  if (pBaseAsMaterialInstance && !pCesiumData) {
    const FStaticParameterSet& parameters =
        pBaseAsMaterialInstance->GetStaticParameters();
    const TArray<FStaticMaterialLayersParameter>& layerParameters =
        parameters.MaterialLayersParameters;

    const FStaticMaterialLayersParameter* pCesiumLayers =
        layerParameters.FindByPredicate(
            [](const FStaticMaterialLayersParameter& layerParameter) {
              return layerParameter.ParameterInfo.Name == "Cesium";
            });

    if (pCesiumLayers) {
#if WITH_EDITOR
      FScopedTransaction transaction(
          FText::FromString("Add Cesium User Data to Material"));
      pBaseAsMaterialInstance->Modify();
#endif
      pCesiumData = NewObject<UCesiumMaterialUserData>(
          pBaseAsMaterialInstance,
          NAME_None,
          RF_Public);
      pBaseAsMaterialInstance->AddAssetUserData(pCesiumData);
      pCesiumData->PostEditChangeOwner();
    }
  }
#endif

  if (pCesiumData) {
    SetGltfParameterValues(
        loadResult,
        material,
        pbr,
        pMaterial,
        EMaterialParameterAssociation::LayerParameter,
        0);

    // If there's a "Water" layer, set its parameters
    int32 waterIndex = pCesiumData->LayerNames.Find("Water");
    if (waterIndex >= 0) {
      SetWaterParameterValues(
          loadResult,
          pMaterial,
          EMaterialParameterAssociation::LayerParameter,
          waterIndex);
    }
  }

  pMaterial->TwoSided = true;

  pStaticMesh->AddMaterial(pMaterial);

  pStaticMesh->InitResources();

  // Set up RenderData bounds and LOD data
  pStaticMesh->CalculateExtendedBounds();

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
  pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
#else
  pStaticMesh->GetRenderData()->ScreenSize[0].Default = 1.0f;
#endif
  pStaticMesh->CreateBodySetup();

  // pMesh->UpdateCollisionFromStaticMesh();
  pMesh->GetBodySetup()->CollisionTraceFlag =
      ECollisionTraceFlag::CTF_UseComplexAsSimple;

  if (loadResult.pCollisionMesh) {
#if PHYSICS_INTERFACE_PHYSX
    pMesh->GetBodySetup()->TriMeshes.Add(loadResult.pCollisionMesh);
#else
    pMesh->GetBodySetup()->ChaosTriMeshes.Add(loadResult.pCollisionMesh);
#endif
  }

  // Mark physics meshes created, no matter if we actually have a collision
  // mesh or not. We don't want the editor creating collision meshes itself in
  // the game thread, because that would be slow.
  pMesh->GetBodySetup()->bCreatedPhysicsMeshes = true;

  pMesh->SetMobility(EComponentMobility::Movable);

  // pMesh->bDrawMeshCollisionIfComplex = true;
  // pMesh->bDrawMeshCollisionIfSimple = true;
  pMesh->SetupAttachment(pGltf);
  pMesh->RegisterComponent();
}

namespace {
class HalfConstructedReal : public UCesiumGltfComponent::HalfConstructed {
public:
  virtual ~HalfConstructedReal() {}
  std::vector<LoadModelResult> loadModelResult;
};
} // namespace

/*static*/ std::unique_ptr<UCesiumGltfComponent::HalfConstructed>
UCesiumGltfComponent::CreateOffGameThread(
    const CesiumGltf::Model& Model,
    const glm::dmat4x4& Transform,
    const CreateModelOptions& Options) {
  auto pResult = std::make_unique<HalfConstructedReal>();
  pResult->loadModelResult = loadModelAnyThreadPart(Model, Transform, Options);
  return pResult;
}

/*static*/ UCesiumGltfComponent* UCesiumGltfComponent::CreateOnGameThread(
    AActor* pParentActor,
    std::unique_ptr<HalfConstructed> pHalfConstructed,
    const glm::dmat4x4& cesiumToUnrealTransform,
    UMaterialInterface* pBaseMaterial,
    UMaterialInterface* pBaseWaterMaterial) {
  HalfConstructedReal* pReal =
      static_cast<HalfConstructedReal*>(pHalfConstructed.get());
  std::vector<LoadModelResult>& result = pReal->loadModelResult;
  if (result.size() == 0) {
    return nullptr;
  }

  UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pParentActor);
  Gltf->SetUsingAbsoluteLocation(true);
  Gltf->SetFlags(RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  if (pBaseMaterial) {
    Gltf->BaseMaterial = pBaseMaterial;
  }

  if (pBaseWaterMaterial) {
    Gltf->BaseMaterialWithWater = pBaseWaterMaterial;
  }

  for (LoadModelResult& model : result) {
    loadModelGameThreadPart(Gltf, model, cesiumToUnrealTransform);
  }
  Gltf->SetVisibility(false, true);
  Gltf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  return Gltf;
}

UCesiumGltfComponent::UCesiumGltfComponent() : USceneComponent() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UMaterialInstance> BaseMaterial;
    ConstructorHelpers::FObjectFinder<UMaterialInstance> BaseMaterialWithWater;
    FConstructorStatics()
        : BaseMaterial(TEXT(
              "/CesiumForUnreal/Materials/Instances/MI_CesiumTwoOverlays.MI_CesiumTwoOverlays")),
          BaseMaterialWithWater(TEXT(
              "/CesiumForUnreal/Materials/Instances/MI_CesiumTwoOverlaysAndWater.MI_CesiumTwoOverlaysAndWater")) {
    }
  };
  static FConstructorStatics ConstructorStatics;

  this->BaseMaterial = ConstructorStatics.BaseMaterial.Object;
  this->BaseMaterialWithWater = ConstructorStatics.BaseMaterialWithWater.Object;

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

void UCesiumGltfComponent::AttachRasterTile(
    const Cesium3DTilesSelection::Tile& tile,
    const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
    UTexture2D* pTexture,
    const glm::dvec2& translation,
    const glm::dvec2& scale,
    int32 textureCoordinateID) {
  this->OverlayTiles.Add(FRasterOverlayTile{
      UTF8_TO_TCHAR(rasterTile.getOverlay().getName().c_str()),
      pTexture,
      FLinearColor(translation.x, translation.y, scale.x, scale.y),
      textureCoordinateID});

  this->UpdateRasterOverlays();
}

void UCesiumGltfComponent::DetachRasterTile(
    const Cesium3DTilesSelection::Tile& tile,
    const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
    UTexture2D* pTexture) {
  size_t numBefore = this->OverlayTiles.Num();
  this->OverlayTiles.RemoveAll(
      [/*overlayName =
          UTF8_TO_TCHAR(rasterTile.getOverlay().getName().c_str()),*/
       pTexture](const FRasterOverlayTile& tile) {
        return /*tile.OverlayName == overlayName &&*/
            tile.Texture == pTexture;
      });
  size_t numAfter = this->OverlayTiles.Num();

  if (numBefore - 1 != numAfter) {
    UE_LOG(
        LogCesium,
        VeryVerbose,
        TEXT("Raster tiles detached: %d, pTexture: %d"),
        numBefore - numAfter,
        pTexture);
  }

  this->UpdateRasterOverlays();
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

void UCesiumGltfComponent::FinishDestroy() {
  UE_LOG(LogCesium, VeryVerbose, TEXT("UCesiumGltfComponent::FinishDestroy"));
  Super::FinishDestroy();
}

void UCesiumGltfComponent::UpdateRasterOverlays() {
  for (USceneComponent* pSceneComponent : this->GetAttachChildren()) {
    UCesiumGltfPrimitiveComponent* pPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pSceneComponent);
    if (pPrimitive) {
      UMaterialInstanceDynamic* pMaterial =
          Cast<UMaterialInstanceDynamic>(pPrimitive->GetMaterial(0));

      if (pMaterial->IsPendingKillOrUnreachable()) {
        // Don't try to update the material while it's in the process of being
        // destroyed. This can lead to the render thread freaking out when
        // it's asked to update a parameter for a material that has been
        // marked for garbage collection.
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

      for (FRasterOverlayTile& overlayTile : this->OverlayTiles) {
        pMaterial->SetTextureParameterValue(
            TCHAR_TO_UTF8(*(overlayTile.OverlayName + "_Texture")),
            overlayTile.Texture);
        pMaterial->SetVectorParameterValue(
            TCHAR_TO_UTF8(*(overlayTile.OverlayName + "_TranslationScale")),
            overlayTile.TranslationAndScale);
        pMaterial->SetScalarParameterValue(
            TCHAR_TO_UTF8(
                *(overlayTile.OverlayName + "_TextureCoordinateIndex")),
            pPrimitive->overlayTextureCoordinateIDToUVIndex
                [overlayTile.TextureCoordinateID]);

        // If this material uses material layers and has the Cesium user data,
        // set the parameters on each material layer that maps to this overlay
        // tile.
        if (pCesiumData) {
          for (int32 i = 0; i < pCesiumData->LayerNames.Num(); ++i) {
            if (pCesiumData->LayerNames[i] != overlayTile.OverlayName) {
              continue;
            }

            pMaterial->SetTextureParameterValueByInfo(
                FMaterialParameterInfo(
                    "Texture",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                overlayTile.Texture);
            pMaterial->SetVectorParameterValueByInfo(
                FMaterialParameterInfo(
                    "TranslationScale",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                overlayTile.TranslationAndScale);
            pMaterial->SetScalarParameterValueByInfo(
                FMaterialParameterInfo(
                    "TextureCoordinateIndex",
                    EMaterialParameterAssociation::LayerParameter,
                    i),
                pPrimitive->overlayTextureCoordinateIDToUVIndex
                    [overlayTile.TextureCoordinateID]);
          }
        }
      }
    }
  }
}

#if !PHYSICS_INTERFACE_PHYSX
// This is copied from FChaosDerivedDataCooker::BuildTriangleMeshes in
// C:\Program Files\Epic
// Games\UE_4.26\Engine\Source\Runtime\Engine\Private\PhysicsEngine\Experimental\ChaosDerivedData.cpp.
// We can't use that method directly because it is private. Since we've copied
// it anyway, we've also modified it to be more efficient for our particular
// input data.
static TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
BuildChaosTriangleMeshes(
    bool duplicateVertices,
    const TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices) {
  UE_LOG(
      LogCesium,
      Warning,
      TEXT(
          "The Chaos physics engine is not currently supported by Cesium for Unreal because functionality required to cook meshes at runtime is not available."));
  return nullptr;
}
#endif
