// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVoxelRendererComponent.h"
#include "CalcBounds.h"
#include "Cesium3DTileset.h"
#include "CesiumLifetime.h"
#include "CesiumMaterialUserData.h"
#include "CesiumRuntime.h"
#include "CesiumVoxelMetadataComponent.h"
#include "EncodedFeaturesMetadata.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"
#include "VecMath.h"

#include <Cesium3DTiles/ExtensionContent3dTilesContentVoxels.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Math.h>
#include <variant>

// Sets default values for this component's properties
UCesiumVoxelRendererComponent::UCesiumVoxelRendererComponent()
    : USceneComponent() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UMaterialInstance> DefaultMaterial;
    ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh;
    FConstructorStatics()
        : DefaultMaterial(TEXT(
              "/CesiumForUnreal/Materials/Instances/MI_CesiumVoxel.MI_CesiumVoxel")),
          CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube")) {}
  };
  static FConstructorStatics ConstructorStatics;

  this->DefaultMaterial = ConstructorStatics.DefaultMaterial.Object;
  this->CubeMesh = ConstructorStatics.CubeMesh.Object;
  this->CubeMesh->NeverStream = true;

  PrimaryComponentTick.bCanEverTick = false;
}

UCesiumVoxelRendererComponent::~UCesiumVoxelRendererComponent() {}

void UCesiumVoxelRendererComponent::BeginDestroy() {
  if (this->MeshComponent) {
    // Only handle the destruction of the material instance. The
    // UStaticMeshComponent attached to this component will be destroyed by the
    // call to destroyComponentRecursively in Cesium3DTileset.cpp.
    UMaterialInstanceDynamic* pMaterial =
        Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(0));
    if (pMaterial) {
      CesiumLifetime::destroy(pMaterial);
    }
  }

  // Reset the pointers.
  this->MeshComponent = nullptr;
  this->_pResources.Reset();

  Super::BeginDestroy();
}

namespace {
EVoxelGridShape getVoxelGridShape(
    const Cesium3DTilesSelection::BoundingVolume& boundingVolume) {
  const CesiumGeometry::OrientedBoundingBox* pBox =
      std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume);
  if (pBox) {
    return EVoxelGridShape::Box;
  }

  return EVoxelGridShape::Invalid;
}

void setVoxelBoxProperties(
    UCesiumVoxelRendererComponent* pVoxelComponent,
    UMaterialInstanceDynamic* pVoxelMaterial,
    const CesiumGeometry::OrientedBoundingBox& box) {
  glm::dmat3 halfAxes = box.getHalfAxes();
  pVoxelComponent->HighPrecisionTransform = glm::dmat4(
      glm::dvec4(halfAxes[0], 0) * 0.02,
      glm::dvec4(halfAxes[1], 0) * 0.02,
      glm::dvec4(halfAxes[2], 0) * 0.02,
      glm::dvec4(box.getCenter(), 1));

  // The transform and scale of the box are handled in the component's
  // transform, so there is no need to duplicate it here. Instead, this
  // transform is configured to scale the engine-provided Cube ([-50, 50]) to
  // unit space ([-1, 1]).
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 0"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0.02, 0, 0, 0));
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 1"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0, 0.02, 0, 0));
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 2"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0, 0, 0.02, 0));
}

FCesiumMetadataValue
getMetadataValue(const std::optional<CesiumUtility::JsonValue>& jsonValue) {
  if (!jsonValue)
    return FCesiumMetadataValue();

  if (jsonValue->isArray()) {
    CesiumUtility::JsonValue::Array array = jsonValue->getArray();
    if (array.size() == 0 || array.size() > 4) {
      return FCesiumMetadataValue();
    }

    // Attempt to convert the array to a vec4 (or a value with less dimensions).
    size_t endIndex = FMath::Min(array.size(), (size_t)4);
    TArray<float> values;
    for (size_t i = 0; i < endIndex; i++) {
      values.Add(UCesiumMetadataValueBlueprintLibrary::GetFloat(
          getMetadataValue(array[i]),
          0.0f));
    }

    switch (values.Num()) {
    case 1:
      return FCesiumMetadataValue(values[0]);
    case 2:
      return FCesiumMetadataValue(glm::vec2(values[0], values[1]));
    case 3:
      return FCesiumMetadataValue(glm::vec3(values[0], values[1], values[2]));
    case 4:
      return FCesiumMetadataValue(
          glm::vec4(values[0], values[1], values[2], values[3]));
    default:
      return FCesiumMetadataValue();
    }
  }

  if (jsonValue->isInt64()) {
    return FCesiumMetadataValue(jsonValue->getInt64OrDefault(0));
  }

  if (jsonValue->isUint64()) {
    return FCesiumMetadataValue(jsonValue->getUint64OrDefault(0));
  }

  if (jsonValue->isDouble()) {
    return FCesiumMetadataValue(jsonValue->getDoubleOrDefault(0.0));
  }

  return FCesiumMetadataValue();
}

// uint32 getMaximumTextureMemory(
//     const FCesiumVoxelClassDescription* pDescription,
//     const glm::uvec3& gridDimensions,
//     uint64_t tileCount) {
//   int32_t pixelSize = 0;
//
//   if (pDescription) {
//     for (const FCesiumPropertyAttributePropertyDescription& Property :
//          pDescription->Properties) {
//       EncodedFeaturesMetadata::EncodedPixelFormat pixelFormat =
//           EncodedFeaturesMetadata::getPixelFormat(
//               Property.EncodingDetails.Type,
//               Property.EncodingDetails.ComponentType);
//       pixelSize = FMath::Max(
//           pixelSize,
//           pixelFormat.bytesPerChannel * pixelFormat.channels);
//     }
//   }
//
//   return (uint32)pixelSize * gridDimensions.x * gridDimensions.y *
//          gridDimensions.z * tileCount;
// }

} // namespace

/*static*/ UMaterialInstanceDynamic*
UCesiumVoxelRendererComponent::CreateVoxelMaterial(
    UCesiumVoxelRendererComponent* pVoxelComponent,
    const FVector& dimensions,
    const FVector& paddingBefore,
    const FVector& paddingAfter,
    ACesium3DTileset* pTilesetActor,
    const Cesium3DTiles::Class* pVoxelClass,
    const FCesiumVoxelClassDescription* pDescription,
    const Cesium3DTilesSelection::BoundingVolume& boundingVolume) {
  UMaterialInterface* pMaterial = pTilesetActor->GetMaterial();

  UMaterialInstanceDynamic* pVoxelMaterial = UMaterialInstanceDynamic::Create(
      pMaterial ? pMaterial : pVoxelComponent->DefaultMaterial,
      nullptr,
      FName("VoxelMaterial"));
  pVoxelMaterial->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  EVoxelGridShape shape = pVoxelComponent->Options.gridShape;

  pVoxelMaterial->SetTextureParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Octree"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      pVoxelComponent->_pResources->GetOctreeTexture());
  pVoxelMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Constant"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      uint8(shape));

  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Grid Dimensions"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      dimensions);
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Padding Before"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      paddingBefore);
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Padding After"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      paddingAfter);

  if (shape == EVoxelGridShape::Box) {
    const CesiumGeometry::OrientedBoundingBox* pBox =
        std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume);
    assert(pBox != nullptr);
    setVoxelBoxProperties(pVoxelComponent, pVoxelMaterial, *pBox);
  }

  if (pDescription && pVoxelClass) {
    for (const auto propertyIt : pVoxelClass->properties) {
      FString UnrealName(propertyIt.first.c_str());

      for (const FCesiumPropertyAttributePropertyDescription& Property :
           pDescription->Properties) {
        if (Property.Name != UnrealName) {
          continue;
        }

        FString PropertyName =
            EncodedFeaturesMetadata::createHlslSafeName(Property.Name);

        pVoxelMaterial->SetTextureParameterValueByInfo(
            FMaterialParameterInfo(
                FName(PropertyName),
                EMaterialParameterAssociation::LayerParameter,
                0),
            pVoxelComponent->_pResources->GetDataTexture(Property.Name));

        if (Property.PropertyDetails.bHasScale) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyScaleSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.scale),
              1);
        }

        if (Property.PropertyDetails.bHasOffset) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyOffsetSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.offset),
              0);
        }

        if (Property.PropertyDetails.bHasNoDataValue) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyNoDataSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.noData),
              0);
        }

        if (Property.PropertyDetails.bHasDefaultValue) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyDefaultValueSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.defaultProperty),
              0);
        }
      }
    }

    pVoxelMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            UTF8_TO_TCHAR("Tile Count"),
            EMaterialParameterAssociation::LayerParameter,
            0),
        pVoxelComponent->_pResources->GetTileCount());
  }

  return pVoxelMaterial;
}

/*static*/ UCesiumVoxelRendererComponent* UCesiumVoxelRendererComponent::Create(
    ACesium3DTileset* pTilesetActor,
    const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
    const Cesium3DTilesSelection::Tile& rootTile,
    const Cesium3DTiles::ExtensionContent3dTilesContentVoxels& voxelExtension,
    const FCesiumVoxelClassDescription* pDescription) {
  if (!pTilesetActor) {
    return nullptr;
  }

  const std::string& voxelClassId = voxelExtension.classProperty;
  if (tilesetMetadata.schema->classes.find(voxelClassId) ==
      tilesetMetadata.schema->classes.end()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Tileset %s contains voxels, but cannot find the metadata class that describes its contents."),
        *pTilesetActor->GetName())
    return nullptr;
  }

  // Validate voxel grid dimensions.
  const std::vector<int64_t> dimensions = voxelExtension.dimensions;
  if (dimensions.size() < 3 || dimensions[0] <= 0 || dimensions[1] <= 0 ||
      dimensions[2] <= 0) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Tileset %s contains voxels but has invalid dimensions."),
        *pTilesetActor->GetName())
    return nullptr;
  }

  // Validate voxel grid padding, if present.
  glm::uvec3 paddingBefore(0);
  glm::uvec3 paddingAfter(0);

  if (voxelExtension.padding) {
    const std::vector<int64_t> before = voxelExtension.padding->before;
    if (before.size() != 3 || before[0] < 0 || before[1] < 0 || before[2] < 0) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "Tileset %s has invalid value for padding.before in its voxel extension."),
          *pTilesetActor->GetName())
      return nullptr;
    }

    const std::vector<int64_t> after = voxelExtension.padding->after;
    if (after.size() != 3 || after[0] < 0 || after[1] < 0 || after[2] < 0) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Tileset %s has invalid value for padding.after in its voxel extension."),
          *pTilesetActor->GetName())
      return nullptr;
    }

    paddingBefore = {before[0], before[1], before[2]};
    paddingAfter = {after[0], after[1], after[2]};
  }

  // Check that bounding volume is supported.
  const Cesium3DTilesSelection::BoundingVolume& boundingVolume =
      rootTile.getBoundingVolume();
  EVoxelGridShape shape = getVoxelGridShape(boundingVolume);
  if (shape == EVoxelGridShape::Invalid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Tileset %s has a root bounding volume that is not supported for voxels."),
        *pTilesetActor->GetName())
    return nullptr;
  }

  const Cesium3DTiles::Class* pVoxelClass =
      &tilesetMetadata.schema->classes.at(voxelClassId);
  assert(pVoxelClass != nullptr);

  UCesiumVoxelRendererComponent* pVoxelComponent =
      NewObject<UCesiumVoxelRendererComponent>(pTilesetActor);
  pVoxelComponent->SetMobility(pTilesetActor->GetRootComponent()->Mobility);
  pVoxelComponent->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pVoxelComponent->_pTileset = pTilesetActor;

  UStaticMeshComponent* pVoxelMesh =
      NewObject<UStaticMeshComponent>(pVoxelComponent);
  pVoxelMesh->SetStaticMesh(pVoxelComponent->CubeMesh);
  pVoxelMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pVoxelMesh->SetMobility(pVoxelComponent->Mobility);
  pVoxelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  FCustomDepthParameters customDepthParameters =
      pTilesetActor->GetCustomDepthParameters();

  pVoxelMesh->SetRenderCustomDepth(customDepthParameters.RenderCustomDepth);
  pVoxelMesh->SetCustomDepthStencilWriteMask(
      customDepthParameters.CustomDepthStencilWriteMask);
  pVoxelMesh->SetCustomDepthStencilValue(
      customDepthParameters.CustomDepthStencilValue);
  pVoxelMesh->bCastDynamicShadow = false;

  pVoxelMesh->SetupAttachment(pVoxelComponent);
  pVoxelMesh->RegisterComponent();

  pVoxelComponent->MeshComponent = pVoxelMesh;

  // The expected size of the incoming glTF attributes depends on padding and
  // voxel grid shape.
  glm::uvec3 dataDimensions =
      glm::uvec3(dimensions[0], dimensions[1], dimensions[2]) + paddingBefore +
      paddingAfter;
  if (shape == EVoxelGridShape::Box || shape == EVoxelGridShape::Cylinder) {
    // Account for y-up in glTF -> z-up in 3D Tiles.
    dataDimensions =
        glm::uvec3(dataDimensions.x, dataDimensions.z, dataDimensions.y);
  }

  uint32 requestedTextureMemory =
      FVoxelResources::DefaultDataTextureMemoryBytes;

  // uint64_t knownTileCount = 0;
  // if (tilesetMetadata.metadata) {
  //   const Cesium3DTiles::MetadataEntity& metadata =
  //   *tilesetMetadata.metadata;
  //   // TODO: This should find the property by "TILESET_TILE_COUNT"
  //   if (metadata.properties.find("tileCount") != metadata.properties.end()) {
  //     const CesiumUtility::JsonValue& value =
  //         metadata.properties.at("tileCount");
  //     if (value.isInt64()) {
  //       knownTileCount = value.getInt64OrDefault(0);
  //     } else if (value.isUint64()) {
  //       knownTileCount = value.getUint64OrDefault(0);
  //     }
  //   }
  // }

  // if (knownTileCount > 0) {
  //  uint32 maximumTextureMemory =
  //      getMaximumTextureMemory(pDescription, dataDimensions, knownTileCount);
  //  requestedTextureMemory = FMath::Min(
  //      maximumTextureMemory,
  //      FVoxelResources::MaximumDataTextureMemoryBytes);
  //}

  pVoxelComponent->_pResources = MakeUnique<FVoxelResources>(
      pDescription,
      shape,
      dataDimensions,
      pVoxelMesh->GetScene()->GetFeatureLevel(),
      requestedTextureMemory);

  CreateGltfOptions::CreateVoxelOptions& options = pVoxelComponent->Options;
  options.pTilesetExtension = &voxelExtension;
  options.pVoxelClass = pVoxelClass;
  options.gridShape = shape;
  options.voxelCount = dataDimensions.x * dataDimensions.y * dataDimensions.z;

  UMaterialInstanceDynamic* pMaterial =
      UCesiumVoxelRendererComponent::CreateVoxelMaterial(
          pVoxelComponent,
          FVector(dimensions[0], dimensions[1], dimensions[2]),
          FVector(paddingBefore.x, paddingBefore.y, paddingBefore.z),
          FVector(paddingAfter.x, paddingAfter.y, paddingAfter.z),
          pTilesetActor,
          pVoxelClass,
          pDescription,
          boundingVolume);
  pVoxelMesh->SetMaterial(0, pMaterial);

  const glm::dmat4& cesiumToUnrealTransform =
      pTilesetActor->GetCesiumTilesetToUnrealRelativeWorldTransform();
  pVoxelComponent->UpdateTransformFromCesium(cesiumToUnrealTransform);

  return pVoxelComponent;
}

void UCesiumVoxelRendererComponent::UpdateTiles(
    const std::vector<Cesium3DTilesSelection::Tile::Pointer>& VisibleTiles,
    const std::vector<double>& VisibleTileScreenSpaceErrors) {
  this->_pResources->Update(VisibleTiles, VisibleTileScreenSpaceErrors);
}

void UCesiumVoxelRendererComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  FTransform transform = FTransform(VecMath::createMatrix(
      CesiumToUnrealTransform * this->HighPrecisionTransform));

  if (this->MeshComponent->Mobility == EComponentMobility::Movable) {
    // For movable objects, move the component in the normal way, but don't
    // generate collisions along the way. Teleporting physics is imperfect,
    // but it's the best available option.
    this->MeshComponent->SetRelativeTransform(
        transform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
  } else {
    // Unreal will yell at us for calling SetRelativeTransform on a static
    // object, but we still need to adjust (accurately!) for origin rebasing
    // and georeference changes. It's "ok" to move a static object in this way
    // because, we assume, the globe and globe-oriented lights, etc. are
    // moving too, so in a relative sense the object isn't actually moving.
    // This isn't a perfect assumption, of course.
    this->MeshComponent->SetRelativeTransform_Direct(transform);
    this->MeshComponent->UpdateComponentToWorld();
    this->MeshComponent->MarkRenderTransformDirty();
  }
}
