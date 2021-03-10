// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfComponent.h"
#include "Async/Async.h"
#include "Cesium3DTiles/Gltf.h"
#include "CesiumGltf/AccessorView.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshTypes.h"
#include "PhysicsEngine/BodySetup.h"
#include "SpdlogUnrealLoggerSink.h"
#include "StaticMeshResources.h"
#include "UObject/ConstructorHelpers.h"
#include "UnrealConversions.h"
#include <iostream>
#if PHYSICS_INTERFACE_PHYSX
#include "IPhysXCooking.h"
#else
#include "Chaos/CollisionConvexMesh.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "ChaosDerivedDataUtil.h"
#endif
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"
#include "CesiumGltf/Reader.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/joinToString.h"
#include "PixelFormat.h"
#include "StaticMeshOperations.h"
#include "UCesiumGltfPrimitiveComponent.h"
#include "mikktspace.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <cstddef>
#include <stb_image_resize.h>

using namespace CesiumGltf;

static uint32_t nextMaterialId = 0;

struct LoadTextureResult {
  FTexturePlatformData* pTextureData;
  TextureAddress addressX;
  TextureAddress addressY;
  TextureFilter filter;
};

struct LoadModelResult {
  FStaticMeshRenderData* RenderData;
  const CesiumGltf::Model* pModel;
  const CesiumGltf::Material* pMaterial;
  glm::dmat4x4 transform;
#if PHYSICS_INTERFACE_PHYSX
  PxTriangleMesh* pCollisionMesh;
#else
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
      pCollisionMesh;
#endif
  std::string name;

  std::optional<LoadTextureResult> baseColorTexture;
  std::optional<LoadTextureResult> metallicRoughnessTexture;
  std::optional<LoadTextureResult> normalTexture;
  std::optional<LoadTextureResult> emissiveTexture;
  std::optional<LoadTextureResult> occlusionTexture;
  std::unordered_map<std::string, uint32_t> textureCoordinateParameters;
};

// Initialize with a static function instead of inline to avoid an
// internal compiler error in MSVC v14.27.29110.
static glm::dmat4 createGltfAxesToCesiumAxes() {
  // https://github.com/CesiumGS/3d-tiles/tree/master/specification#gltf-transforms
  return glm::dmat4(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(0.0, -1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));
}

glm::dmat4 gltfAxesToCesiumAxes = createGltfAxesToCesiumAxes();

static const std::string rasterOverlay0 = "_CESIUMOVERLAY_0";

template <class T, class TIndexAccessor>
static uint32_t updateTextureCoordinates(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TIndexAccessor& indicesView,
    const std::optional<T>& texture,
    std::unordered_map<uint32_t, uint32_t>& textureCoordinateMap) {
  if (!texture) {
    return 0;
  }

  return updateTextureCoordinates(
      model,
      primitive,
      vertices,
      indicesView,
      "TEXCOORD_" + std::to_string(texture.value().texCoord),
      textureCoordinateMap);
}

template <class TIndexAccessor>
uint32_t updateTextureCoordinates(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    TArray<FStaticMeshBuildVertex>& vertices,
    const TIndexAccessor& indicesView,
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

  for (int64_t i = 0; i < static_cast<int64_t>(indicesView.size()); ++i) {
    FStaticMeshBuildVertex& vertex = vertices[i];
    TIndexAccessor::value_type vertexIndex = indicesView[i];
    if (vertexIndex >= 0 && vertexIndex < uvAccessor.size()) {
      vertex.UVs[textureCoordinateIndex] = uvAccessor[vertexIndex];
    } else {
      vertex.UVs[textureCoordinateIndex] = FVector2D(0.0f, 0.0f);
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
  SMikkTSpaceInterface MikkTInterface;
  MikkTInterface.m_getNormal = mikkGetNormal;
  MikkTInterface.m_getNumFaces = mikkGetNumFaces;
  MikkTInterface.m_getNumVerticesOfFace = mikkGetNumVertsOfFace;
  MikkTInterface.m_getPosition = mikkGetPosition;
  MikkTInterface.m_getTexCoord = mikkGetTexCoord;
  MikkTInterface.m_setTSpaceBasic = mikkSetTSpaceBasic;
  MikkTInterface.m_setTSpace = nullptr;

  SMikkTSpaceContext MikkTContext;
  MikkTContext.m_pInterface = &MikkTInterface;
  MikkTContext.m_pUserData = (void*)(&vertices);
  MikkTContext.m_bIgnoreDegenerates = false;
  genTangSpaceDefault(&MikkTContext);
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

template <typename TIndexView> struct ColorVisitor {
  TArray<FStaticMeshBuildVertex>& StaticMeshBuildVertices;
  const TIndexView& indicesView;

  bool operator()(AccessorView<nullptr_t>&& invalidView) { return false; }

  template <typename TColorView> bool operator()(TColorView&& colorView) {
    bool success = true;

    for (int64_t i = 0;
         success && i < static_cast<int64_t>(this->indicesView.size());
         ++i) {
      FStaticMeshBuildVertex& vertex = this->StaticMeshBuildVertices[i];
      TIndexView::value_type vertexIndex = this->indicesView[i];
      if (vertexIndex >= colorView.size()) {
        success = false;
      } else {
        success =
            ColorVisitor::convertColor(colorView[vertexIndex], vertex.Color);
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

static FTexturePlatformData*
createTexturePlatformData(int32 sizeX, int32 sizeY, EPixelFormat format) {
  if (sizeX > 0 && sizeY > 0 &&
      (sizeX % GPixelFormats[format].BlockSizeX) == 0 &&
      (sizeY % GPixelFormats[format].BlockSizeY) == 0) {
    FTexturePlatformData* pTexturePlatformData = new FTexturePlatformData();
    pTexturePlatformData->SizeX = sizeX;
    pTexturePlatformData->SizeY = sizeY;
    pTexturePlatformData->PixelFormat = format;

    // Allocate first mipmap.
    int32 NumBlocksX = sizeX / GPixelFormats[format].BlockSizeX;
    int32 NumBlocksY = sizeY / GPixelFormats[format].BlockSizeY;
    FTexture2DMipMap* Mip = new FTexture2DMipMap();
    pTexturePlatformData->Mips.Add(Mip);
    Mip->SizeX = sizeX;
    Mip->SizeY = sizeY;
    Mip->BulkData.Lock(LOCK_READ_WRITE);
    Mip->BulkData.Realloc(
        NumBlocksX * NumBlocksY * GPixelFormats[format].BlockBytes);
    Mip->BulkData.Unlock();

    return pTexturePlatformData;
  } else {
    return nullptr;
  }
}

template <class T>
static std::optional<LoadTextureResult> loadTexture(
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
    return std::nullopt;
  }

  const CesiumGltf::Texture& texture =
      model.textures[gltfTexture.value().index];
  if (texture.source < 0 || texture.source >= model.images.size()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Texture source index must be non-negative and less than %d, but is %d"),
        model.images.size(),
        texture.source);
    return std::nullopt;
  }

  const CesiumGltf::Image& image = model.images[texture.source];
  LoadTextureResult result{};
  result.pTextureData = createTexturePlatformData(
      image.cesium.width,
      image.cesium.height,
      PF_R8G8B8A8);
  if (!result.pTextureData) {
    return std::nullopt;
  }

  const CesiumGltf::Sampler* pSampler =
      CesiumGltf::Model::getSafe(&model.samplers, texture.sampler);
  if (pSampler) {
    switch (pSampler->wrapS) {
    case CesiumGltf::Sampler::WrapS::CLAMP_TO_EDGE:
      result.addressX = TextureAddress::TA_Clamp;
      break;
    case CesiumGltf::Sampler::WrapS::MIRRORED_REPEAT:
      result.addressX = TextureAddress::TA_Mirror;
      break;
    case CesiumGltf::Sampler::WrapS::REPEAT:
      result.addressX = TextureAddress::TA_Wrap;
      break;
    }

    switch (pSampler->wrapT) {
    case CesiumGltf::Sampler::WrapT::CLAMP_TO_EDGE:
      result.addressY = TextureAddress::TA_Clamp;
      break;
    case CesiumGltf::Sampler::WrapT::MIRRORED_REPEAT:
      result.addressY = TextureAddress::TA_Mirror;
      break;
    case CesiumGltf::Sampler::WrapT::REPEAT:
      result.addressY = TextureAddress::TA_Wrap;
      break;
    }

    // Unreal Engine's available filtering modes are only nearest, bilinear, and
    // trilinear, and are not specified separately for minification and
    // magnification. So we get as close as we can.
    if (!pSampler->minFilter && !pSampler->magFilter) {
      result.filter = TextureFilter::TF_Default;
    } else if (
        (!pSampler->minFilter ||
         pSampler->minFilter == CesiumGltf::Sampler::MinFilter::NEAREST) &&
        (!pSampler->magFilter ||
         pSampler->magFilter == CesiumGltf::Sampler::MagFilter::NEAREST)) {
      result.filter = TextureFilter::TF_Nearest;
    } else if (pSampler->minFilter) {
      switch (pSampler->minFilter.value()) {
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
        result.filter = TextureFilter::TF_Trilinear;
        break;
      default:
        result.filter = TextureFilter::TF_Bilinear;
        break;
      }
    } else if (pSampler->magFilter) {
      result.filter =
          pSampler->magFilter.value() == CesiumGltf::Sampler::MagFilter::LINEAR
              ? TextureFilter::TF_Bilinear
              : TextureFilter::TF_Nearest;
    }
  } else {
    // glTF spec: "When undefined, a sampler with repeat wrapping and auto
    // filtering should be used."
    result.addressX = TextureAddress::TA_Wrap;
    result.addressY = TextureAddress::TA_Wrap;
    result.filter = TextureFilter::TF_Default;
  }

  void* pTextureData = static_cast<unsigned char*>(
      result.pTextureData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
  FMemory::Memcpy(
      pTextureData,
      image.cesium.pixelData.data(),
      image.cesium.pixelData.size());

  if (result.filter == TextureFilter::TF_Trilinear) {
    // Generate mip levels.
    // TODO: do this on the GPU?
    int32_t width = image.cesium.width;
    int32_t height = image.cesium.height;

    while (width > 1 || height > 1) {
      FTexture2DMipMap* pLevel = new FTexture2DMipMap();
      result.pTextureData->Mips.Add(pLevel);

      pLevel->SizeX = width >> 1;
      if (pLevel->SizeX < 1)
        pLevel->SizeX = 1;
      pLevel->SizeY = height >> 1;
      if (pLevel->SizeY < 1)
        pLevel->SizeY = 1;

      pLevel->BulkData.Lock(LOCK_READ_WRITE);

      void* pMipData =
          pLevel->BulkData.Realloc(pLevel->SizeX * pLevel->SizeY * 4);
      if (!stbir_resize_uint8(
              static_cast<const unsigned char*>(pTextureData),
              width,
              height,
              0,
              static_cast<unsigned char*>(pMipData),
              pLevel->SizeX,
              pLevel->SizeY,
              0,
              4)) {
        // Failed to generate mip level, use bilinear filtering instead.
        result.filter = TextureFilter::TF_Bilinear;
        for (int32_t i = 1; i < result.pTextureData->Mips.Num(); ++i) {
          result.pTextureData->Mips[i].BulkData.Unlock();
        }
        result.pTextureData->Mips.RemoveAt(
            1,
            result.pTextureData->Mips.Num() - 1);
        break;
      }

      width = pLevel->SizeX;
      height = pLevel->SizeY;
      pTextureData = pMipData;
    }
  }

  // Unlock all levels
  for (int32_t i = 0; i < result.pTextureData->Mips.Num(); ++i) {
    result.pTextureData->Mips[i].BulkData.Unlock();
  }

  return result;
}

template <class TIndexAccessor>
static void loadPrimitive(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::dmat4x4& transform,
#if PHYSICS_INTERFACE_PHYSX
    IPhysXCooking* pPhysXCooking,
#endif
    const CesiumGltf::Accessor& positionAccessor,
    const CesiumGltf::AccessorView<FVector>& positionView,
    const TIndexAccessor& indicesView) {
  if (primitive.mode != CesiumGltf::MeshPrimitive::Mode::TRIANGLES) {
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
    name = urlIt->second.getString("glTF");
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

  FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();
  RenderData->AllocateLODResources(1);

  FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

  const std::vector<double>& min = positionAccessor.min;
  const std::vector<double>& max = positionAccessor.max;

  glm::dvec3 minPosition = glm::dvec3(min[0], min[1], min[2]);
  glm::dvec3 maxPosition = glm::dvec3(max[0], max[1], max[2]);

  FBox aaBox(
      FVector(minPosition.x, minPosition.y, minPosition.z),
      FVector(maxPosition.x, maxPosition.y, maxPosition.z));

  FBoxSphereBounds BoundingBoxAndSphere;
  aaBox.GetCenterAndExtents(
      BoundingBoxAndSphere.Origin,
      BoundingBoxAndSphere.BoxExtent);
  BoundingBoxAndSphere.SphereRadius = 0.0f;

  TArray<FStaticMeshBuildVertex> StaticMeshBuildVertices;
  StaticMeshBuildVertices.SetNum(indicesView.size());

  // The static mesh we construct will _not_ be indexed, even if the incoming
  // glTF is. This allows us to compute flat normals if the glTF doesn't include
  // them already, and it allows us to compute a correct tangent space basis
  // according to the MikkTSpace algorithm when tangents are not included in the
  // glTF.

  for (int64_t i = 0; i < static_cast<int64_t>(indicesView.size()); ++i) {
    FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
    TIndexAccessor::value_type vertexIndex = indicesView[i];
    vertex.Position = positionView[vertexIndex];
    vertex.UVs[0] = FVector2D(0.0f, 0.0f);
    vertex.UVs[2] = FVector2D(0.0f, 0.0f);
    BoundingBoxAndSphere.SphereRadius = FMath::Max(
        (vertex.Position - BoundingBoxAndSphere.Origin).Size(),
        BoundingBoxAndSphere.SphereRadius);
  }

  // TangentX: Tangent
  // TangentY: Bi-tangent
  // TangentZ: Normal

  auto normalAccessorIt = primitive.attributes.find("NORMAL");
  if (normalAccessorIt != primitive.attributes.end()) {
    int normalAccessorID = normalAccessorIt->second;
    CesiumGltf::AccessorView<FVector> normalAccessor(model, normalAccessorID);

    for (int64_t i = 0; i < static_cast<int64_t>(indicesView.size()); ++i) {
      FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
      TIndexAccessor::value_type vertexIndex = indicesView[i];
      vertex.TangentZ = normalAccessor[vertexIndex];
    }
  } else {
    // Compute flat normals
    for (int64_t i = 0; i < static_cast<int64_t>(indicesView.size()); i += 3) {
      FStaticMeshBuildVertex& v0 = StaticMeshBuildVertices[i];
      FStaticMeshBuildVertex& v1 = StaticMeshBuildVertices[i + 1];
      FStaticMeshBuildVertex& v2 = StaticMeshBuildVertices[i + 2];

      FVector v01 = v1.Position - v0.Position;
      FVector v02 = v2.Position - v0.Position;
      FVector normal = FVector::CrossProduct(v01, v02);

      v0.TangentZ = normal.GetSafeNormal();
      v1.TangentZ = v0.TangentZ;
      v2.TangentZ = v0.TangentZ;
    }
  }

  auto tangentAccessorIt = primitive.attributes.find("TANGENT");
  if (tangentAccessorIt != primitive.attributes.end()) {
    int tangentAccessorID = tangentAccessorIt->second;
    CesiumGltf::AccessorView<FVector4> tangentAccessor(
        model,
        tangentAccessorID);

    for (int64_t i = 0; i < static_cast<int64_t>(indicesView.size()); ++i) {
      FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
      TIndexAccessor::value_type vertexIndex = indicesView[i];
      const FVector4& tangent = tangentAccessor[vertexIndex];
      vertex.TangentX = tangent;
      vertex.TangentY =
          FVector::CrossProduct(vertex.TangentZ, vertex.TangentX) * tangent.W;
    }
  } else {
    // Use mikktspace to calculate the tangents
    computeTangentSpace(StaticMeshBuildVertices);
  }

  bool hasVertexColors = false;

  auto colorAccessorIt = primitive.attributes.find("COLOR_0");
  if (colorAccessorIt != primitive.attributes.end()) {
    int colorAccessorID = colorAccessorIt->second;
    hasVertexColors = CesiumGltf::createAccessorView(
        model,
        colorAccessorID,
        ColorVisitor<TIndexAccessor>{StaticMeshBuildVertices, indicesView});
  }

  LODResources.bHasColorVertexData = hasVertexColors;

  // We need to copy the texture coordinates associated with each texture (if
  // any) into the the appropriate UVs slot in FStaticMeshBuildVertex.

  int materialID = primitive.material;
  const CesiumGltf::Material& material =
      materialID >= 0 && materialID < model.materials.size()
          ? model.materials[materialID]
          : defaultMaterial;
  const CesiumGltf::MaterialPBRMetallicRoughness& pbrMetallicRoughness =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  std::unordered_map<uint32_t, uint32_t> textureCoordinateMap;

  primitiveResult.baseColorTexture =
      loadTexture(model, pbrMetallicRoughness.baseColorTexture);
  primitiveResult.metallicRoughnessTexture =
      loadTexture(model, pbrMetallicRoughness.metallicRoughnessTexture);
  primitiveResult.normalTexture = loadTexture(model, material.normalTexture);
  primitiveResult.occlusionTexture =
      loadTexture(model, material.occlusionTexture);
  primitiveResult.emissiveTexture =
      loadTexture(model, material.emissiveTexture);
  primitiveResult
      .textureCoordinateParameters["baseColorTextureCoordinateIndex"] =
      updateTextureCoordinates(
          model,
          primitive,
          StaticMeshBuildVertices,
          indicesView,
          pbrMetallicRoughness.baseColorTexture,
          textureCoordinateMap);
  primitiveResult
      .textureCoordinateParameters["metallicRoughnessTextureCoordinateIndex"] =
      updateTextureCoordinates(
          model,
          primitive,
          StaticMeshBuildVertices,
          indicesView,
          pbrMetallicRoughness.metallicRoughnessTexture,
          textureCoordinateMap);
  primitiveResult.textureCoordinateParameters["normalTextureCoordinateIndex"] =
      updateTextureCoordinates(
          model,
          primitive,
          StaticMeshBuildVertices,
          indicesView,
          material.normalTexture,
          textureCoordinateMap);
  primitiveResult
      .textureCoordinateParameters["occlusionTextureCoordinateIndex"] =
      updateTextureCoordinates(
          model,
          primitive,
          StaticMeshBuildVertices,
          indicesView,
          material.occlusionTexture,
          textureCoordinateMap);
  primitiveResult
      .textureCoordinateParameters["emissiveTextureCoordinateIndex"] =
      updateTextureCoordinates(
          model,
          primitive,
          StaticMeshBuildVertices,
          indicesView,
          material.emissiveTexture,
          textureCoordinateMap);

  // Currently only one set of raster overlay texture coordinates is supported.
  // TODO: Support more texture coordinate sets (e.g. web mercator and
  // geographic)
  primitiveResult.textureCoordinateParameters["overlayTextureCoordinateIndex"] =
      updateTextureCoordinates(
          model,
          primitive,
          StaticMeshBuildVertices,
          indicesView,
          rasterOverlay0,
          textureCoordinateMap);

  RenderData->Bounds = BoundingBoxAndSphere;

  LODResources.VertexBuffers.PositionVertexBuffer.Init(StaticMeshBuildVertices);

  FColorVertexBuffer& ColorVertexBuffer =
      LODResources.VertexBuffers.ColorVertexBuffer;
  if (hasVertexColors) {
    ColorVertexBuffer.Init(StaticMeshBuildVertices);
  } else if (indicesView.size() > 0) {
    ColorVertexBuffer.InitFromSingleColor(FColor::White, indicesView.size());
  }

  LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(
      StaticMeshBuildVertices,
      textureCoordinateMap.size() == 0 ? 1 : textureCoordinateMap.size());

  FStaticMeshLODResources::FStaticMeshSectionArray& Sections =
      LODResources.Sections;
  FStaticMeshSection& section = Sections.AddDefaulted_GetRef();
  section.bEnableCollision = true;

  section.NumTriangles = StaticMeshBuildVertices.Num() / 3;
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = StaticMeshBuildVertices.Num() - 1;
  section.bEnableCollision = true;
  section.bCastShadow = true;

  TArray<uint32> indices;
  indices.SetNum(StaticMeshBuildVertices.Num());

  // Note that we're reversing the order of the indices, because the change from
  // the glTF right-handed to the Unreal left-handed coordinate system reverses
  // the winding order.
  for (int32 i = 0; i < indices.Num(); ++i) {
    indices[i] = indices.Num() - i - 1;
  }

  LODResources.IndexBuffer.SetIndices(
      indices,
      indices.Num() > std::numeric_limits<uint16>::max()
          ? EIndexBufferStride::Type::Force32Bit
          : EIndexBufferStride::Type::Force16Bit);

  LODResources.bHasDepthOnlyIndices = false;
  LODResources.bHasReversedIndices = false;
  LODResources.bHasReversedDepthOnlyIndices = false;
  LODResources.bHasAdjacencyInfo = false;

  primitiveResult.pModel = &model;
  primitiveResult.RenderData = RenderData;
  primitiveResult.transform = transform;
  primitiveResult.pMaterial = &material;

  section.MaterialIndex = 0;

#if PHYSICS_INTERFACE_PHYSX
  primitiveResult.pCollisionMesh = nullptr;

  if (pPhysXCooking) {
    // TODO: use PhysX interface directly so we don't need to copy the vertices
    // (it takes a stride parameter).
    TArray<FVector> vertices;
    vertices.SetNum(StaticMeshBuildVertices.Num());

    for (size_t i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
      vertices[i] = StaticMeshBuildVertices[i].Position;
    }

    TArray<FTriIndices> physicsIndices;
    physicsIndices.SetNum(StaticMeshBuildVertices.Num() / 3);

    // Reversing triangle winding order here, too.
    for (size_t i = 0; i < StaticMeshBuildVertices.Num() / 3; ++i) {
      physicsIndices[i].v0 = i * 3 + 2;
      physicsIndices[i].v1 = i * 3 + 1;
      physicsIndices[i].v2 = i * 3;
    }

    pPhysXCooking->CreateTriMesh(
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
    primitiveResult.pCollisionMesh =
        BuildChaosTriangleMeshes(StaticMeshBuildVertices, indices);
  }
#endif

  result.push_back(std::move(primitiveResult));
}

static void loadPrimitive(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const CesiumGltf::MeshPrimitive& primitive,
    const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
    ,
    IPhysXCooking* pPhysXCooking
#endif
) {
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
#if PHYSICS_INTERFACE_PHYSX
        pPhysXCooking,
#endif
        *pPositionAccessor,
        positionView,
        syntheticIndexBuffer);
  } else {
    const CesiumGltf::Accessor& indexAccessorGltf =
        model.accessors[primitive.indices];
    if (indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT) {
      CesiumGltf::AccessorView<uint16_t> indexAccessor(
          model,
          primitive.indices);
      loadPrimitive(
          result,
          model,
          mesh,
          primitive,
          transform,
#if PHYSICS_INTERFACE_PHYSX
          pPhysXCooking,
#endif
          *pPositionAccessor,
          positionView,
          indexAccessor);
    } else if (
        indexAccessorGltf.componentType ==
        CesiumGltf::Accessor::ComponentType::UNSIGNED_INT) {
      CesiumGltf::AccessorView<uint32_t> indexAccessor(
          model,
          primitive.indices);
      loadPrimitive(
          result,
          model,
          mesh,
          primitive,
          transform,
#if PHYSICS_INTERFACE_PHYSX
          pPhysXCooking,
#endif
          *pPositionAccessor,
          positionView,
          indexAccessor);
    } else {
      // TODO: report unsupported index type.
      return;
    }
  }
}

static void loadMesh(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Mesh& mesh,
    const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
    ,
    IPhysXCooking* pPhysXCooking
#endif
) {
  for (const CesiumGltf::MeshPrimitive& primitive : mesh.primitives) {
    loadPrimitive(
        result,
        model,
        mesh,
        primitive,
        transform
#if PHYSICS_INTERFACE_PHYSX
        ,
        pPhysXCooking
#endif
    );
  }
}

static void loadNode(
    std::vector<LoadModelResult>& result,
    const CesiumGltf::Model& model,
    const CesiumGltf::Node& node,
    const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
    ,
    IPhysXCooking* pPhysXCooking
#endif
) {
  glm::dmat4x4 nodeTransform = transform;

  if (node.matrix.size() > 0) {
    const std::vector<double>& matrix = node.matrix;

    glm::dmat4x4 nodeTransformGltf(
        glm::dvec4(matrix[0], matrix[1], matrix[2], matrix[3]),
        glm::dvec4(matrix[4], matrix[5], matrix[6], matrix[7]),
        glm::dvec4(matrix[8], matrix[9], matrix[10], matrix[11]),
        glm::dvec4(matrix[12], matrix[13], matrix[14], matrix[15]));

    nodeTransform = nodeTransform * nodeTransformGltf;
  } else if (
      node.translation.size() > 0 || node.rotation.size() > 0 ||
      node.scale.size() > 0) {
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
    loadMesh(
        result,
        model,
        mesh,
        nodeTransform
#if PHYSICS_INTERFACE_PHYSX
        ,
        pPhysXCooking
#endif
    );
  }

  for (int childNodeId : node.children) {
    if (childNodeId >= 0 && childNodeId < model.nodes.size()) {
      loadNode(
          result,
          model,
          model.nodes[childNodeId],
          nodeTransform
#if PHYSICS_INTERFACE_PHYSX
          ,
          pPhysXCooking
#endif
      );
    }
  }
}

static std::vector<LoadModelResult> loadModelAnyThreadPart(
    const CesiumGltf::Model& model,
    const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
    ,
    IPhysXCooking* pPhysXCooking
#endif
) {
  std::vector<LoadModelResult> result;

  glm::dmat4x4 rootTransform;

  auto rtcCenterIt = model.extras.find("RTC_CENTER");
  if (rtcCenterIt != model.extras.end()) {
    const CesiumGltf::JsonValue& rtcCenter = rtcCenterIt->second;
    const std::vector<CesiumGltf::JsonValue>* pArray =
        std::get_if<CesiumGltf::JsonValue::Array>(&rtcCenter.value);
    if (pArray && pArray->size() == 3) {
      glm::dmat4x4 rtcTransform(
          glm::dvec4(1.0, 0.0, 0.0, 0.0),
          glm::dvec4(0.0, 1.0, 0.0, 0.0),
          glm::dvec4(0.0, 0.0, 1.0, 0.0),
          glm::dvec4(
              (*pArray)[0].getNumber(0.0),
              (*pArray)[1].getNumber(0.0),
              (*pArray)[2].getNumber(0.0),
              1.0));
      rootTransform = transform * rtcTransform * gltfAxesToCesiumAxes;
    } else {
      rootTransform = transform * gltfAxesToCesiumAxes;
    }
  } else {
    rootTransform = transform * gltfAxesToCesiumAxes;
  }

  if (model.scene >= 0 && model.scene < model.scenes.size()) {
    // Show the default scene
    const CesiumGltf::Scene& defaultScene = model.scenes[model.scene];
    for (int nodeId : defaultScene.nodes) {
      loadNode(
          result,
          model,
          model.nodes[nodeId],
          rootTransform
#if PHYSICS_INTERFACE_PHYSX
          ,
          pPhysXCooking
#endif
      );
    }
  } else if (model.scenes.size() > 0) {
    // There's no default, so show the first scene
    const CesiumGltf::Scene& defaultScene = model.scenes[0];
    for (int nodeId : defaultScene.nodes) {
      loadNode(
          result,
          model,
          model.nodes[nodeId],
          rootTransform
#if PHYSICS_INTERFACE_PHYSX
          ,
          pPhysXCooking
#endif
      );
    }
  } else if (model.nodes.size() > 0) {
    // No scenes at all, use the first node as the root node.
    loadNode(
        result,
        model,
        model.nodes[0],
        rootTransform
#if PHYSICS_INTERFACE_PHYSX
        ,
        pPhysXCooking
#endif
    );
  } else if (model.meshes.size() > 0) {
    // No nodes either, show all the meshes.
    for (const CesiumGltf::Mesh& mesh : model.meshes) {
      loadMesh(
          result,
          model,
          mesh,
          rootTransform
#if PHYSICS_INTERFACE_PHYSX
          ,
          pPhysXCooking
#endif
      );
    }
  }

  return result;
}

bool applyTexture(
    UMaterialInstanceDynamic* pMaterial,
    FName parameterName,
    const std::optional<LoadTextureResult>& loadedTexture) {
  if (!loadedTexture) {
    return false;
  }

  UTexture2D* pTexture =
      NewObject<UTexture2D>(GetTransientPackage(), NAME_None, RF_Transient);

  pTexture->PlatformData = loadedTexture->pTextureData;
  pTexture->AddressX = loadedTexture->addressX;
  pTexture->AddressY = loadedTexture->addressY;
  pTexture->Filter = loadedTexture->filter;
  pTexture->UpdateResource();
  pMaterial->SetTextureParameterValue(parameterName, pTexture);
  return true;
}

static void loadModelGameThreadPart(
    UCesiumGltfComponent* pGltf,
    LoadModelResult& loadResult,
    const glm::dmat4x4& cesiumToUnrealTransform) {
  UCesiumGltfPrimitiveComponent* pMesh =
      NewObject<UCesiumGltfPrimitiveComponent>(
          pGltf,
          FName(loadResult.name.c_str()));
  pMesh->HighPrecisionNodeTransform = loadResult.transform;
  pMesh->UpdateTransformFromCesium(cesiumToUnrealTransform);

  pMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  pMesh->bUseDefaultCollision = true;
  // pMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
  pMesh->SetFlags(RF_Transient);

  UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
  pMesh->SetStaticMesh(pStaticMesh);

  pStaticMesh->bIsBuiltAtRuntime = true;
  pStaticMesh->NeverStream = true;
  pStaticMesh->RenderData =
      TUniquePtr<FStaticMeshRenderData>(loadResult.RenderData);

  const CesiumGltf::Model& model = *loadResult.pModel;
  const CesiumGltf::Material& material =
      loadResult.pMaterial ? *loadResult.pMaterial : defaultMaterial;

  const CesiumGltf::MaterialPBRMetallicRoughness& pbr =
      material.pbrMetallicRoughness ? material.pbrMetallicRoughness.value()
                                    : defaultPbrMetallicRoughness;

  const FName ImportedSlotName(
      *(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));
  UMaterialInstanceDynamic* pMaterial;

  switch (material.alphaMode) {
  case CesiumGltf::Material::AlphaMode::BLEND:
    // TODO
    pMaterial = UMaterialInstanceDynamic::Create(
        pGltf->OpacityMaskMaterial,
        nullptr,
        ImportedSlotName);
    break;
  case CesiumGltf::Material::AlphaMode::MASK:
    pMaterial = UMaterialInstanceDynamic::Create(
        pGltf->OpacityMaskMaterial,
        nullptr,
        ImportedSlotName);
    break;
  case CesiumGltf::Material::AlphaMode::OPAQUE:
  default:
    pMaterial = UMaterialInstanceDynamic::Create(
        pGltf->BaseMaterial,
        nullptr,
        ImportedSlotName);
    break;
  }

  pMaterial->OpacityMaskClipValue = material.alphaCutoff;

  for (auto& textureCoordinateSet : loadResult.textureCoordinateParameters) {
    pMaterial->SetScalarParameterValue(
        textureCoordinateSet.first.c_str(),
        textureCoordinateSet.second);
  }

  if (pbr.baseColorFactor.size() >= 3) {
    pMaterial->SetVectorParameterValue(
        "baseColorFactor",
        FVector(
            pbr.baseColorFactor[0],
            pbr.baseColorFactor[1],
            pbr.baseColorFactor[2]));
  }
  pMaterial->SetScalarParameterValue("metallicFactor", pbr.metallicFactor);
  pMaterial->SetScalarParameterValue("roughnessFactor", pbr.roughnessFactor);
  pMaterial->SetScalarParameterValue("opacityMask", 1.0);

  applyTexture(pMaterial, "baseColorTexture", loadResult.baseColorTexture);
  applyTexture(
      pMaterial,
      "metallicRoughnessTexture",
      loadResult.metallicRoughnessTexture);
  applyTexture(pMaterial, "normalTexture", loadResult.normalTexture);
  bool hasEmissiveTexture =
      applyTexture(pMaterial, "emissiveTexture", loadResult.emissiveTexture);
  applyTexture(pMaterial, "occlusionTexture", loadResult.occlusionTexture);

  if (material.emissiveFactor.size() >= 3) {
    pMaterial->SetVectorParameterValue(
        "emissiveFactor",
        FVector(
            material.emissiveFactor[0],
            material.emissiveFactor[1],
            material.emissiveFactor[2]));
  } else if (hasEmissiveTexture) {
    // When we have an emissive texture but not a factor, we need to use a
    // factor of vec3(1.0). The default, vec3(0.0), would disable the emission
    // from the texture.
    pMaterial->SetVectorParameterValue(
        "emissiveFactor",
        FVector(1.0f, 1.0f, 1.0f));
  }

  pMaterial->TwoSided = true;

  pStaticMesh->AddMaterial(pMaterial);

  pStaticMesh->InitResources();

  // Set up RenderData bounds and LOD data
  pStaticMesh->CalculateExtendedBounds();

  pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
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
    pMesh->GetBodySetup()->bCreatedPhysicsMeshes = true;
  }

  pMesh->SetMobility(EComponentMobility::Movable);

  // pMesh->bDrawMeshCollisionIfComplex = true;
  // pMesh->bDrawMeshCollisionIfSimple = true;
  pMesh->SetupAttachment(pGltf);
  pMesh->RegisterComponent();
}

/*static*/ void UCesiumGltfComponent::CreateOffGameThread(
    AActor* pActor,
    const CesiumGltf::Model& model,
    const glm::dmat4x4& transform,
    TFunction<void(UCesiumGltfComponent*)> callback) {
  std::vector<LoadModelResult> result = loadModelAnyThreadPart(
      model,
      transform
#if PHYSICS_INTERFACE_PHYSX
      ,
      nullptr
#endif
  );

  AsyncTask(
      ENamedThreads::GameThread,
      [pActor, callback, result{std::move(result)}]() mutable {
        UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pActor);
        for (LoadModelResult& model : result) {
          loadModelGameThreadPart(
              Gltf,
              model,
              CesiumTransforms::unrealToOrFromCesium *
                  CesiumTransforms::scaleToUnrealWorld);
        }
        Gltf->SetVisibility(false, true);
        callback(Gltf);
      });
}

namespace {
class HalfConstructedReal : public UCesiumGltfComponent::HalfConstructed {
public:
  virtual ~HalfConstructedReal() = default;
  std::vector<LoadModelResult> loadModelResult;
};
} // namespace

/*static*/ std::unique_ptr<UCesiumGltfComponent::HalfConstructed>
UCesiumGltfComponent::CreateOffGameThread(
    const CesiumGltf::Model& model,
    const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
    ,
    IPhysXCooking* pPhysXCooking
#endif
) {
  auto pResult = std::make_unique<HalfConstructedReal>();
  pResult->loadModelResult = std::move(loadModelAnyThreadPart(
      model,
      transform
#if PHYSICS_INTERFACE_PHYSX
      ,
      pPhysXCooking
#endif
      ));
  return pResult;
}

/*static*/ UCesiumGltfComponent* UCesiumGltfComponent::CreateOnGameThread(
    AActor* pParentActor,
    std::unique_ptr<HalfConstructed> pHalfConstructed,
    const glm::dmat4x4& cesiumToUnrealTransform,
    UMaterial* pBaseMaterial) {
  HalfConstructedReal* pReal =
      static_cast<HalfConstructedReal*>(pHalfConstructed.get());
  std::vector<LoadModelResult>& result = pReal->loadModelResult;
  if (result.size() == 0) {
    return nullptr;
  }

  UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pParentActor);
  Gltf->SetUsingAbsoluteLocation(true);
  Gltf->SetFlags(RF_Transient);

  if (pBaseMaterial) {
    Gltf->BaseMaterial = pBaseMaterial;
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
    ConstructorHelpers::FObjectFinder<UMaterial> BaseMaterial;
    ConstructorHelpers::FObjectFinder<UMaterial> OpacityMaskMaterial;
    FConstructorStatics()
        : BaseMaterial(TEXT(
              "/CesiumForUnreal/GltfMaterialWithOverlays.GltfMaterialWithOverlays")),
          OpacityMaskMaterial(TEXT(
              "/CesiumForUnreal/GltfMaterialOpacityMask.GltfMaterialOpacityMask")) {
    }
  };
  static FConstructorStatics ConstructorStatics;

  this->BaseMaterial = ConstructorStatics.BaseMaterial.Object;
  this->OpacityMaskMaterial = ConstructorStatics.OpacityMaskMaterial.Object;

  PrimaryComponentTick.bCanEverTick = false;
}

UCesiumGltfComponent::~UCesiumGltfComponent() {
  UE_LOG(LogCesium, VeryVerbose, TEXT("~UCesiumGltfComponent"));
}

void UCesiumGltfComponent::LoadModel(const FString& Url) {
  if (this->LoadedUrl == Url) {
    UE_LOG(LogCesium, VeryVerbose, TEXT("Model URL unchanged"))
    return;
  }

  if (this->Mesh) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Deleting old model from %s"),
        *this->LoadedUrl);
    this->Mesh->DetachFromComponent(
        FDetachmentTransformRules::KeepRelativeTransform);
    this->Mesh->UnregisterComponent();
    this->Mesh->DestroyComponent(false);
    this->Mesh = nullptr;
  }

  UE_LOG(LogCesium, Verbose, TEXT("Loading model from %s"), *Url)

  this->LoadedUrl = Url;

  FHttpModule& httpModule = FHttpModule::Get();
  FHttpRequestRef request = httpModule.CreateRequest();
  request->SetURL(Url);

  // TODO: This delegate will be invoked in the game thread, which is totally
  // unnecessary and a waste of the game thread's time. Ideally we'd avoid the
  // main thread entirely, but for now we just dispatch the real work to another
  // thread.
  request->OnProcessRequestComplete().BindUObject(
      this,
      &UCesiumGltfComponent::ModelRequestComplete);
  request->ProcessRequest();
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
    const Cesium3DTiles::Tile& tile,
    const Cesium3DTiles::RasterOverlayTile& rasterTile,
    UTexture2D* pTexture,
    const CesiumGeometry::Rectangle& textureCoordinateRectangle,
    const glm::dvec2& translation,
    const glm::dvec2& scale) {
  if (this->_overlayTiles.Num() == 0) {
    // First overlay tile, generate texture coordinates
    // TODO
  }

  this->_overlayTiles.Add(FRasterOverlayTile{
      pTexture,
      FLinearColor(
          textureCoordinateRectangle.minimumX,
          textureCoordinateRectangle.minimumY,
          textureCoordinateRectangle.maximumX,
          textureCoordinateRectangle.maximumY),
      FLinearColor(translation.x, translation.y, scale.x, scale.y)});

  if (this->_overlayTiles.Num() > 3) {
    UE_LOG(LogCesium, Warning, TEXT("Too many raster overlays"));
  }

  this->updateRasterOverlays();
}

void UCesiumGltfComponent::DetachRasterTile(
    const Cesium3DTiles::Tile& tile,
    const Cesium3DTiles::RasterOverlayTile& rasterTile,
    UTexture2D* pTexture,
    const CesiumGeometry::Rectangle& textureCoordinateRectangle) {
  size_t numBefore = this->_overlayTiles.Num();
  this->_overlayTiles.RemoveAll(
      [pTexture, &textureCoordinateRectangle](const FRasterOverlayTile& tile) {
        return tile.pTexture == pTexture &&
               tile.textureCoordinateRectangle.Equals(FLinearColor(
                   textureCoordinateRectangle.minimumX,
                   textureCoordinateRectangle.minimumY,
                   textureCoordinateRectangle.maximumX,
                   textureCoordinateRectangle.maximumY));
      });
  size_t numAfter = this->_overlayTiles.Num();

  if (numBefore - 1 != numAfter) {
    UE_LOG(
        LogCesium,
        VeryVerbose,
        TEXT(
            "Raster tiles detached: %d, pTexture: %d, minX: %f, minY: %f, maxX: %f, maxY: %f"),
        numBefore - numAfter,
        pTexture,
        textureCoordinateRectangle.minimumX,
        textureCoordinateRectangle.minimumY,
        textureCoordinateRectangle.maximumX,
        textureCoordinateRectangle.maximumY);
  }

  this->updateRasterOverlays();
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

void UCesiumGltfComponent::ModelRequestComplete(
    FHttpRequestPtr request,
    FHttpResponsePtr response,
    bool x) {
  const TArray<uint8>& content = response->GetContent();
  if (content.Num() < 4) {
    return;
  }

  // TODO: is it reasonable to use the global thread pool for this?
  TFuture<void> future = Async(EAsyncExecution::ThreadPool, [this, content] {
    gsl::span<const std::byte> data(
        reinterpret_cast<const std::byte*>(content.GetData()),
        content.Num());
    std::unique_ptr<CesiumGltf::ModelReaderResult> pLoadResult =
        std::make_unique<CesiumGltf::ModelReaderResult>(
            std::move(CesiumGltf::readModel(data)));

    if (!pLoadResult->warnings.empty()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Warnings while loading glTF: %s"),
          *utf8_to_wstr(
              CesiumUtility::joinToString(pLoadResult->warnings, "\n- ")));
    }

    if (!pLoadResult->errors.empty()) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT("Errors while loading glTF: %s"),
          *utf8_to_wstr(
              CesiumUtility::joinToString(pLoadResult->errors, "\n- ")));
    }

    if (!pLoadResult->model) {
      UE_LOG(LogCesium, Error, TEXT("glTF model could not be loaded."));
      return;
    }

    CesiumGltf::Model& model = pLoadResult->model.value();

    std::vector<LoadModelResult> result = loadModelAnyThreadPart(
        model,
        glm::dmat4x4(1.0)
#if PHYSICS_INTERFACE_PHYSX
            ,
        nullptr
#endif
    );

    AsyncTask(
        ENamedThreads::GameThread,
        [this,
         pLoadResult{std::move(pLoadResult)},
         result{std::move(result)}]() mutable {
          for (LoadModelResult& model : result) {
            loadModelGameThreadPart(
                this,
                model,
                CesiumTransforms::unrealToOrFromCesium *
                    CesiumTransforms::scaleToUnrealWorld);
          }
        });
  });
}

void UCesiumGltfComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  // this->Mesh->DestroyComponent();
  // this->Mesh = nullptr;
}

void UCesiumGltfComponent::updateRasterOverlays() {
  for (USceneComponent* pSceneComponent : this->GetAttachChildren()) {
    UCesiumGltfPrimitiveComponent* pPrimitive =
        Cast<UCesiumGltfPrimitiveComponent>(pSceneComponent);
    if (pPrimitive) {
      UMaterialInstanceDynamic* pMaterial =
          Cast<UMaterialInstanceDynamic>(pPrimitive->GetMaterial(0));

      if (pMaterial->IsPendingKillOrUnreachable()) {
        // Don't try to update the material while it's in the process of being
        // destroyed. This can lead to the render thread freaking out when it's
        // asked to update a parameter for a material that has been marked for
        // garbage collection.
        continue;
      }

      for (size_t i = 0; i < this->_overlayTiles.Num(); ++i) {
        FRasterOverlayTile& overlayTile = this->_overlayTiles[i];
        std::string is = std::to_string(i + 1);
        pMaterial->SetTextureParameterValue(
            ("OverlayTexture" + is).c_str(),
            overlayTile.pTexture);

        if (!overlayTile.pTexture) {
          // The texture is null so don't use it.
          pMaterial->SetVectorParameterValue(
              ("OverlayRect" + is).c_str(),
              FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
        } else {
          pMaterial->SetVectorParameterValue(
              ("OverlayRect" + is).c_str(),
              overlayTile.textureCoordinateRectangle);
        }

        pMaterial->SetVectorParameterValue(
            ("OverlayTranslationScale" + is).c_str(),
            overlayTile.translationAndScale);
      }

      for (size_t i = this->_overlayTiles.Num(); i < 3; ++i) {
        std::string is = std::to_string(i + 1);
        pMaterial->SetTextureParameterValue(
            ("OverlayTexture" + is).c_str(),
            nullptr);
        pMaterial->SetVectorParameterValue(
            ("OverlayRect" + is).c_str(),
            FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
        pMaterial->SetVectorParameterValue(
            ("OverlayTranslationScale" + is).c_str(),
            FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
      }

      pMaterial->SetScalarParameterValue(
          "opacityMask",
          this->_overlayTiles.Num() > 0 ? 0.0 : 1.0);
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
    const TArray<FStaticMeshBuildVertex>& vertices,
    const TArray<uint32>& indices) {
  TArray<FVector> FinalVerts;
  FinalVerts.Reserve(vertices.Num());

  for (const FStaticMeshBuildVertex& vertex : vertices) {
    FinalVerts.Add(vertex.Position);
  }

  // Push indices into one flat array
  TArray<int32> FinalIndices;
  FinalIndices.Reserve(indices.Num());
  for (int32 i = 0; i < indices.Num(); i += 3) {
    // question: It seems like unreal triangles are CW, but couldn't find
    // confirmation for this
    FinalIndices.Add(indices[i + 1]);
    FinalIndices.Add(indices[i]);
    FinalIndices.Add(indices[i + 2]);
  }

  TArray<int32> OutFaceRemap;

  // if (EnableMeshClean)
  { Chaos::CesiumCleanTriMeshes(FinalVerts, FinalIndices, &OutFaceRemap); }

  // Build particle list #BG Maybe allow TParticles to copy vectors?
  Chaos::TParticles<Chaos::FReal, 3> TriMeshParticles;
  TriMeshParticles.AddParticles(FinalVerts.Num());

  const int32 NumVerts = FinalVerts.Num();
  for (int32 VertIndex = 0; VertIndex < NumVerts; ++VertIndex) {
    TriMeshParticles.X(VertIndex) = FinalVerts[VertIndex];
  }

  // Build chaos triangle list. #BGTODO Just make the clean function take these
  // types instead of double copying
  const int32 NumTriangles = FinalIndices.Num() / 3;
  bool bHasMaterials =
      true; // InParams.TriangleMeshDesc.MaterialIndices.Num() > 0;
  TArray<uint16> MaterialIndices;

  auto LambdaHelper = [&](auto& Triangles) {
    if (bHasMaterials) {
      MaterialIndices.Reserve(NumTriangles);
    }

    Triangles.Reserve(NumTriangles);
    for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles;
         ++TriangleIndex) {
      // Only add this triangle if it is valid
      const int32 BaseIndex = TriangleIndex * 3;
      const bool bIsValidTriangle = Chaos::FConvexBuilder::IsValidTriangle(
          FinalVerts[FinalIndices[BaseIndex]],
          FinalVerts[FinalIndices[BaseIndex + 1]],
          FinalVerts[FinalIndices[BaseIndex + 2]]);

      // TODO: Figure out a proper way to handle this. Could these edges get
      // sewn together? Is this important?
      // if (ensureMsgf(bIsValidTriangle,
      // TEXT("FChaosDerivedDataCooker::BuildTriangleMeshes(): Trimesh attempted
      // cooked with invalid triangle!")));
      if (bIsValidTriangle) {
        Triangles.Add(Chaos::TVector<int32, 3>(
            FinalIndices[BaseIndex],
            FinalIndices[BaseIndex + 1],
            FinalIndices[BaseIndex + 2]));

        if (bHasMaterials) {
          // if (EnableMeshClean)
          {
            if (!ensure(OutFaceRemap.IsValidIndex(TriangleIndex))) {
              MaterialIndices.Empty();
              bHasMaterials = false;
            } else {
              MaterialIndices.Add(0);
              // const int32 OriginalIndex = OutFaceRemap[TriangleIndex];

              // if
              // (ensure(InParams.TriangleMeshDesc.MaterialIndices.IsValidIndex(OriginalIndex)))
              //{
              //	MaterialIndices.Add(InParams.TriangleMeshDesc.MaterialIndices[OriginalIndex]);
              //}
              // else
              //{
              //	MaterialIndices.Empty();
              //	bHasMaterials = false;
              //}
            }
          }
          // else
          //{
          //	if
          //(ensure(InParams.TriangleMeshDesc.MaterialIndices.IsValidIndex(TriangleIndex)))
          //	{
          //		MaterialIndices.Add(InParams.TriangleMeshDesc.MaterialIndices[TriangleIndex]);
          //	}
          //	else
          //	{
          //		MaterialIndices.Empty();
          //		bHasMaterials = false;
          //	}
          //}
        }
      }
    }

    TUniquePtr<TArray<int32>> OutFaceRemapPtr =
        MakeUnique<TArray<int32>>(OutFaceRemap);
    return MakeShared<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>(
        MoveTemp(TriMeshParticles),
        MoveTemp(Triangles),
        MoveTemp(MaterialIndices),
        MoveTemp(OutFaceRemapPtr));
  };

  if (FinalVerts.Num() < TNumericLimits<uint16>::Max()) {
    TArray<Chaos::TVector<uint16, 3>> TrianglesSmallIdx;
    return LambdaHelper(TrianglesSmallIdx);
  } else {
    TArray<Chaos::TVector<int32, 3>> TrianglesLargeIdx;
    return LambdaHelper(TrianglesLargeIdx);
  }
}
#endif
