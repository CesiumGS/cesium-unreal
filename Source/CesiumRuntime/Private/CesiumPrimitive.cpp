// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumPrimitive.h"
#include "Cesium3DTilesStyle.h"
#include "CesiumCommon.h"

FVector3f scalePositionForUnreal(const FVector3f& position) {
  FVector3f result;
  result.X = position.X * CesiumPrimitiveData::positionScaleFactor;
  result.Y = -position.Y * CesiumPrimitiveData::positionScaleFactor;
  result.Z = position.Z * CesiumPrimitiveData::positionScaleFactor;
  return result;
}

void CesiumPrimitiveData::destroy() {
  this->features = FCesiumPrimitiveFeatures();
  this->metadata = FCesiumPrimitiveMetadata();
  this->encodedFeatures = EncodedFeaturesMetadata::EncodedPrimitiveFeatures();
  this->encodedMetadata = EncodedFeaturesMetadata::EncodedPrimitiveMetadata();

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  this->metadata_DEPRECATED = FCesiumMetadataPrimitive();
  this->encodedMetadata_DEPRECATED.reset();
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  this->pTilesetActor = nullptr;
  this->pMeshPrimitive = nullptr;

  std::unordered_map<int32_t, uint32_t> emptyTexCoordMap;
  this->gltfToUnrealTexCoordMap.swap(emptyTexCoordMap);

  std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>
      emptyAccessorMap;
  this->texCoordAccessorMap.swap(emptyAccessorMap);
}

const CesiumGltf::MeshPrimitive* ICesiumPrimitive::GetMeshPrimitive() const {
  return this->getPrimitiveData().pMeshPrimitive;
}

const FCesiumPrimitiveFeatures& ICesiumPrimitive::GetPrimitiveFeatures() const {
  return this->getPrimitiveData().features;
}

const FCesiumPrimitiveMetadata& ICesiumPrimitive::GetPrimitiveMetadata() const {
  return this->getPrimitiveData().metadata;
}

void ICesiumPrimitive::ApplyStyle(
    UObject* pObject,
    const FCesiumModelMetadata& modelMetadata) {
  if (!IsValid(pObject)) {
    return;
  }

  const FCesiumPrimitiveFeatures& features = this->GetPrimitiveFeatures();
  const TArray<FCesiumFeatureIdSet>& featureIdSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(features);

  const TArray<EncodedFeaturesMetadata::EncodedFeatureIdSet>&
      encodedFeatureIdSets =
          this->getPrimitiveData().encodedFeatures.featureIdSets;

  for (const auto& encodedFeatureIdSet : encodedFeatureIdSets) {
    const FCesiumFeatureIdSet& featureIdSet =
        featureIdSets[encodedFeatureIdSet.index];

    const FCesiumPropertyTable& propertyTable =
        UCesiumModelMetadataBlueprintLibrary::GetPropertyTable(
            modelMetadata,
            UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(
                featureIdSet));

    TSet<int64> uniqueFeatureIds =
        UCesiumFeatureIdSetBlueprintLibrary::GetUniqueFeatureIDs(featureIdSet);

    int64 count = UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
        propertyTable);

    std::vector<std::byte> colorResult(count * sizeof(uint8_t) * 4);
    uint8_t* pColorData = reinterpret_cast<uint8_t*>(colorResult.data());

    TScriptInterface<ICesium3DTilesStylingCallbacks> pInterface = pObject;

    for (int64 id : uniqueFeatureIds) {
      FColor result = ICesium3DTilesStylingCallbacks::Execute_EvaluateColor(
          pInterface.GetObject(),
          propertyTable,
          id);
      uint8_t* pWrite = pColorData + (id * sizeof(uint8_t) * 4);
      pWrite[0] = result.R;
      pWrite[1] = result.G;
      pWrite[2] = result.B;
      pWrite[3] = result.A;
    }

    FString name("_FEATURE_ID_0_Color");
    UTexture2D** ppColorTexture =
        this->getPrimitiveData().styling.colorTextures.Find(name);
    UTexture2D* pColorTexture = ppColorTexture ? *ppColorTexture : nullptr;

    int64 textureDimension = 0;

    if (pColorTexture) {
      FUpdateTextureRegion2D region;
      region.DestX = 0;
      region.DestY = 0;
      region.Width = pColorTexture->GetResource()->GetSizeX();
      region.Height = pColorTexture->GetResource()->GetSizeY();
      region.SrcX = 0;
      region.SrcY = 0;

      // Pitch = size in bytes of each row of the source image
      uint32 sourcePitch = region.Width * 4;

      ENQUEUE_RENDER_COMMAND(Cesium_UpdateResource)
      ([pResource = pColorTexture->GetResource(),
        result = std::move(colorResult),
        region,
        sourcePitch](FRHICommandListImmediate& RHICmdList) {
        RHICmdList.UpdateTexture2D(
            pResource->TextureRHI,
            0,
            region,
            sourcePitch,
            reinterpret_cast<const uint8*>(result.data()));
      });
    } else {
      int64 floorSqrtFeatureCount = glm::sqrt(count);
      int64 textureDimension =
          (floorSqrtFeatureCount * floorSqrtFeatureCount == count)
              ? floorSqrtFeatureCount
              : (floorSqrtFeatureCount + 1);

      FTextureResource* pResource = FCesiumTextureResource::CreateEmpty(
                                        TextureGroup::TEXTUREGROUP_8BitData,
                                        textureDimension,
                                        textureDimension,
                                        1, /* Depth */
                                        EPixelFormat::PF_R8G8B8A8,
                                        TextureFilter::TF_Nearest,
                                        TextureAddress::TA_Clamp,
                                        TextureAddress::TA_Clamp,
                                        false)
                                        .Release();

      UTexture2D* pColorTexture = NewObject<UTexture2D>(
          GetTransientPackage(),
          MakeUniqueObjectName(
              GetTransientPackage(),
              UTexture2D::StaticClass(),
              "HighlightColorTexture"),
          RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

      pColorTexture->AddressX = TextureAddress::TA_Clamp;
      pColorTexture->AddressY = TextureAddress::TA_Clamp;
      pColorTexture->Filter = TextureFilter::TF_Nearest;
      pColorTexture->LODGroup = TextureGroup::TEXTUREGROUP_8BitData;
      pColorTexture->SRGB = false;
      pColorTexture->NeverStream = true;

      if (!pColorTexture || !pResource) {
        // UE_LOG(LogCesium, Error, TEXT("Could not create texture."));
        return;
      }

      pColorTexture->SetResource(pResource);

      ENQUEUE_RENDER_COMMAND(Cesium_InitResource)(
          [pColorTexture,
           pResource = pColorTexture->GetResource(),
           result = std::move(colorResult),
           textureDimension](FRHICommandListImmediate& RHICmdList) {
            pResource->SetTextureReference(
                pColorTexture->TextureReference.TextureReferenceRHI);
            pResource->InitResource(FRHICommandListImmediate::Get());
            uint32 DestPitch;
            void* pDestination = RHILockTexture2D(
                pResource->TextureRHI,
                0,
                RLM_WriteOnly,
                DestPitch,
                false);
            uint32 sourcePitch = textureDimension * sizeof(uint8_t) * 4;
            CopyTextureData2D(
                result.data(),
                pDestination,
                textureDimension,
                EPixelFormat::PF_R8G8B8A8,
                sourcePitch,
                DestPitch);

            RHIUnlockTexture2D(pResource->TextureRHI, 0, false);
          });
      this->getPrimitiveData().styling.colorTextures.Add(name, pColorTexture);
    }
  }
}
