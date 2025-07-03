// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVoxelRendererComponent.h"
#include "CalcBounds.h"
#include "Cesium3DTileset.h"
#include "CesiumGltfComponent.h"
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

  Super::BeginDestroy();
}

bool UCesiumVoxelRendererComponent::IsReadyForFinishDestroy() {
  if (this->_pOctree.IsValid() && !this->_pOctree->canBeDestroyed()) {
    return false;
  }

  if (this->_pDataTextures.IsValid()) {
    this->_pDataTextures->pollLoadingSlots();
    return this->_pDataTextures->canBeDestroyed();
  }

  return Super::IsReadyForFinishDestroy();
}

namespace {
EVoxelGridShape getVoxelGridShape(
    const Cesium3DTilesSelection::BoundingVolume& boundingVolume) {
  if (std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume)) {
    return EVoxelGridShape::Box;
  }

  return EVoxelGridShape::Invalid;
}

void setVoxelBoxProperties(
    UCesiumVoxelRendererComponent* pVoxelComponent,
    UMaterialInstanceDynamic* pVoxelMaterial,
    const CesiumGeometry::OrientedBoundingBox& box) {
  glm::dmat3 halfAxes = box.getHalfAxes();

  // The engine-provided Cube extends from [-50, 50], so a scale of 1/50 is
  // incorporated into the component's transform to compensate.
  pVoxelComponent->HighPrecisionTransform = glm::dmat4(
      glm::dvec4(halfAxes[0], 0) * 0.02,
      glm::dvec4(halfAxes[1], 0) * 0.02,
      glm::dvec4(halfAxes[2], 0) * 0.02,
      glm::dvec4(box.getCenter(), 1));

  // Distinct from the component's transform above, this scales from the
  // engine-provided Cube's space ([-50, 50]) to a unit space of [-1, 1]. This
  // is specifically used to fit the raymarched cube into the bounds of the
  // explicit cube mesh. In other words, this scale must be applied in-shader
  // to account for the actual mesh's bounds.
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

    // Attempt to convert the array to a vec4 (or a value with less
    // dimensions).
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
      pVoxelComponent->_pOctree->getTexture());
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
            pVoxelComponent->_pDataTextures->getTexture(Property.Name));

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

    const glm::uvec3& tileCount =
        pVoxelComponent->_pDataTextures->getTileCountAlongAxes();
    pVoxelMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            UTF8_TO_TCHAR("Tile Count"),
            EMaterialParameterAssociation::LayerParameter,
            0),
        FVector(tileCount.x, tileCount.y, tileCount.z));
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
            "Tileset %s does not contain the metadata class that is referenced by its voxel content."),
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
        TEXT("Tileset %s has invalid voxel grid dimensions."),
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
  CESIUM_ASSERT(pVoxelClass != nullptr);

  UCesiumVoxelRendererComponent* pVoxelComponent =
      NewObject<UCesiumVoxelRendererComponent>(pTilesetActor);
  pVoxelComponent->SetMobility(pTilesetActor->GetRootComponent()->Mobility);
  pVoxelComponent->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

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
    // Account for the transformation between y-up (glTF) to z-up (3D Tiles).
    dataDimensions =
        glm::uvec3(dataDimensions.x, dataDimensions.z, dataDimensions.y);
  }

  uint32 knownTileCount = 0;
  if (tilesetMetadata.metadata) {
    const Cesium3DTiles::MetadataEntity& metadata = *tilesetMetadata.metadata;
    const Cesium3DTiles::Class& tilesetClass =
        tilesetMetadata.schema->classes.at(metadata.classProperty);
    for (const auto propertyIt : tilesetClass.properties) {
      if (propertyIt.second.semantic == "TILESET_TILE_COUNT") {
        const auto tileCountIt = metadata.properties.find(propertyIt.first);
        if (tileCountIt != metadata.properties.end()) {
          knownTileCount =
              tileCountIt->second.getSafeNumberOrDefault<uint32>(0);
        }
        break;
      }
    }
  }

  if (pDescription && pVoxelMesh->GetScene()) {
    pVoxelComponent->_pDataTextures = MakeUnique<FVoxelMegatextures>(
        *pDescription,
        dataDimensions,
        pVoxelMesh->GetScene()->GetFeatureLevel(),
        knownTileCount);
  }

  uint32 maximumTileCount =
      pVoxelComponent->_pDataTextures
          ? pVoxelComponent->_pDataTextures->getMaximumTileCount()
          : 1;
  pVoxelComponent->_pOctree = MakeUnique<FVoxelOctree>(maximumTileCount);
  pVoxelComponent->_loadedNodeIds.reserve(maximumTileCount);

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

namespace {
template <typename Func>
void forEachRenderableVoxelTile(const auto& tiles, Func&& f) {
  for (size_t i = 0; i < tiles.size(); i++) {
    const Cesium3DTilesSelection::Tile::Pointer& pTile = tiles[i];
    if (!pTile ||
        pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done) {
      continue;
    }

    const Cesium3DTilesSelection::TileContent& content = pTile->getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        content.getRenderContent();
    if (!pRenderContent) {
      continue;
    }

    UCesiumGltfComponent* pGltf = static_cast<UCesiumGltfComponent*>(
        pRenderContent->getRenderResources());
    if (!pGltf) {
      // When a tile does not have render resources (i.e. a glTF), then
      // the resources either have not yet been loaded or prepared,
      // or the tile is from an external tileset and does not directly
      // own renderable content. In both cases, the tile is ignored here.
      continue;
    }

    const TArray<USceneComponent*>& Children = pGltf->GetAttachChildren();
    for (USceneComponent* pChild : Children) {
      UCesiumGltfVoxelComponent* pVoxelComponent =
          Cast<UCesiumGltfVoxelComponent>(pChild);
      if (!pVoxelComponent) {
        continue;
      }

      f(i, pVoxelComponent);
    }
  }
}
} // namespace

void UCesiumVoxelRendererComponent::UpdateTiles(
    const std::vector<Cesium3DTilesSelection::Tile::Pointer>& VisibleTiles,
    const std::vector<double>& VisibleTileScreenSpaceErrors) {
  forEachRenderableVoxelTile(
      VisibleTiles,
      [&VisibleTileScreenSpaceErrors,
       &priorityQueue = this->_visibleTileQueue,
       &pOctree = this->_pOctree](
          size_t index,
          const UCesiumGltfVoxelComponent* pVoxel) {
        double sse = VisibleTileScreenSpaceErrors[index];
        FVoxelOctree::Node* pNode = pOctree->getNode(pVoxel->TileId);
        if (pNode) {
          pNode->lastKnownScreenSpaceError = sse;
        }

        // Don't create the missing node just yet? It may not be added to the
        // tree depending on the priority of other nodes.
        priorityQueue.push({pVoxel, sse, computePriority(sse)});
      });

  if (this->_visibleTileQueue.empty()) {
    return;
  }

  // Sort the existing nodes in the megatexture by highest to lowest priority.
  std::sort(
      this->_loadedNodeIds.begin(),
      this->_loadedNodeIds.end(),
      [&pOctree = this->_pOctree](
          const CesiumGeometry::OctreeTileID& lhs,
          const CesiumGeometry::OctreeTileID& rhs) {
        const FVoxelOctree::Node* pLeft = pOctree->getNode(lhs);
        const FVoxelOctree::Node* pRight = pOctree->getNode(rhs);
        if (!pLeft) {
          return false;
        }
        if (!pRight) {
          return true;
        }
        return computePriority(pLeft->lastKnownScreenSpaceError) >
               computePriority(pRight->lastKnownScreenSpaceError);
      });

  size_t existingNodeCount = this->_loadedNodeIds.size();
  size_t destroyedNodeCount = 0;
  size_t addedNodeCount = 0;

  if (this->_pDataTextures) {
    // For all of the visible nodes...
    for (; !this->_visibleTileQueue.empty(); this->_visibleTileQueue.pop()) {
      const VoxelTileUpdateInfo& currentTile = this->_visibleTileQueue.top();
      const CesiumGeometry::OctreeTileID& currentTileId =
          currentTile.pComponent->TileId;
      FVoxelOctree::Node* pNode = this->_pOctree->getNode(currentTileId);
      if (pNode && pNode->dataIndex >= 0) {
        // Node has already been loaded into the data textures.
        pNode->isDataReady =
            this->_pDataTextures->isSlotLoaded(pNode->dataIndex);
        continue;
      }

      // Otherwise, check that the data textures have the space to add it.
      const UCesiumGltfVoxelComponent* pVoxel = currentTile.pComponent;
      size_t addNodeIndex = 0;
      if (this->_pDataTextures->isFull()) {
        addNodeIndex = existingNodeCount - 1 - destroyedNodeCount;
        if (addNodeIndex >= this->_loadedNodeIds.size()) {
          // This happens when all of the previously loaded nodes have been
          // replaced with new ones.
          continue;
        }

        destroyedNodeCount++;

        const CesiumGeometry::OctreeTileID& lowestPriorityId =
            this->_loadedNodeIds[addNodeIndex];
        FVoxelOctree::Node* pLowestPriorityNode =
            this->_pOctree->getNode(lowestPriorityId);

        // Release the data slot of the lowest priority node.
        this->_pDataTextures->release(pLowestPriorityNode->dataIndex);
        pLowestPriorityNode->dataIndex = -1;
        pLowestPriorityNode->isDataReady = false;

        // Attempt to remove the node and simplify the octree.
        // Will not succeed if the node's siblings are renderable, or if this
        // node contains renderable children.
        this->_needsOctreeUpdate |=
            this->_pOctree->removeNode(lowestPriorityId);
      } else {
        addNodeIndex = existingNodeCount + addedNodeCount;
        addedNodeCount++;
      }

      // Create the node if it does not already exist in the tree.
      bool createdNewNode = this->_pOctree->createNode(currentTileId);
      pNode = this->_pOctree->getNode(currentTileId);
      pNode->lastKnownScreenSpaceError = currentTile.sse;

      pNode->dataIndex = this->_pDataTextures->add(*pVoxel);
      bool addedToDataTexture = (pNode->dataIndex >= 0);
      this->_needsOctreeUpdate |= createdNewNode || addedToDataTexture;

      if (!addedToDataTexture) {
        continue;
      } else if (addNodeIndex < this->_loadedNodeIds.size()) {
        this->_loadedNodeIds[addNodeIndex] = currentTileId;
      } else {
        this->_loadedNodeIds.push_back(currentTileId);
      }
    }

    this->_needsOctreeUpdate |= this->_pDataTextures->pollLoadingSlots();
  } else {
    // If there are no data textures, then for all of the visible nodes...
    for (; !this->_visibleTileQueue.empty(); this->_visibleTileQueue.pop()) {
      const VoxelTileUpdateInfo& currentTile = this->_visibleTileQueue.top();
      const CesiumGeometry::OctreeTileID& currentTileId =
          currentTile.pComponent->TileId;
      // Create the node if it does not already exist in the tree.
      this->_needsOctreeUpdate |= this->_pOctree->createNode(currentTileId);

      FVoxelOctree::Node* pNode = this->_pOctree->getNode(currentTileId);
      pNode->lastKnownScreenSpaceError = currentTile.sse;
      // Set to arbitrary index. This will prompt the tile to render even
      // though it does not actually have data.
      pNode->dataIndex = 0;
      pNode->isDataReady = true;
    }
  }

  if (this->_needsOctreeUpdate) {
    this->_needsOctreeUpdate = !this->_pOctree->updateTexture();
  }
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

double UCesiumVoxelRendererComponent::computePriority(double sse) {
  return 10.0 * sse / (sse + 1.0);
}
