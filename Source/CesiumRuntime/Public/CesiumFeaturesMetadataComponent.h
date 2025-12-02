// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeaturesMetadataDescription.h"
#include "Components/ActorComponent.h"

#if WITH_EDITOR
#include "Materials/MaterialFunctionMaterialLayer.h"
#endif

#include "CesiumFeaturesMetadataComponent.generated.h"

/**
 * @brief A component that can be added to Cesium3DTileset actors to
 * dictate what feature ID sets or metadata to encode for access on the GPU.
 * "Add Properties" allows users to find and select desired feature ID sets and
 * metadata properties. Once a selection is made, "Generate Material" can be
 * used to auto-generated the boiler-plate code to access the selected
 * properties in the Unreal material.
 */
UCLASS(ClassGroup = Cesium, Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumFeaturesMetadataComponent
    : public UActorComponent {
  GENERATED_BODY()

public:
#if WITH_EDITOR
  /**
   * Opens a window to add feature ID sets and metadata properties from the
   * current view of the tileset.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void AddProperties();

  /**
   * This button can be used to create a boiler-plate material layer that
   * exposes the requested metadata properties in the current description. The
   * nodes to access the metadata will be added to TargetMaterialLayer if it
   * exists. Otherwise a new material layer will be created in the /Content/
   * folder and TargetMaterialLayer will be set to the new material layer.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void GenerateMaterial();
#endif

#if WITH_EDITORONLY_DATA
  /**
   * This is the target UMaterialFunctionMaterialLayer that the
   * boiler-plate material generation will use. When pressing
   * "Generate Material", nodes will be added to this material to enable access
   * to the requested metadata. If this is left blank, a new material layer
   * will be created in the /Game/ folder.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialFunctionMaterialLayer* TargetMaterialLayer = nullptr;
#endif

  /**
   * @brief Description of both feature IDs and metadata from a glTF via the
   * EXT_mesh_features, EXT_instance_features, and EXT_structural_metadata
   * extensions. Indicates what parts of the extension should be uploaded to the
   * GPU for access in Unreal materials.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (TitleProperty = "Name",
           DisplayAfter = "TargetMaterialLayer",
           ShowOnlyInnerProperties))
  FCesiumFeaturesMetadataDescription Description;

  // Previously the properties of FCesiumFeaturesMetadataDescription were
  // deconstructed here in order to flatten the Details panel UI. However, the
  // ShowOnlyInnerProperties attribute accomplishes the same thing. These
  // properties are deprecated but migrated over in PostLoad().

  /**
   * Description of the feature ID sets in the visible glTF primitives across
   * the tileset.
   */
  UPROPERTY(
      Meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Use FeatureIdSets on the CesiumFeaturesMetadataDescription's Features instead."))
  TArray<FCesiumFeatureIdSetDescription> FeatureIdSets;

  /**
   * Names of the property textures used by the glTF primitives across the
   * tileset.
   *
   * This should be a subset of the property textures listed in the model
   * metadata. Property textures can be passed to the material even if they are
   * not explicitly used by a glTF primitive, but the primitive may lack the
   * corresponding sets of texture coordinates intended to sample them.
   */
  UPROPERTY(
      Meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Use PropertyTextureNames on the CesiumFeaturesMetadataDescription's PrimitiveMetadata instead."))
  TSet<FString> PropertyTextureNames;

  /**
   * Descriptions of the property tables in the visible glTF
   * models across the tileset.
   */
  UPROPERTY(
      Meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Use PropertyTables on the CesiumFeaturesMetadataDescription's ModelMetadata instead."))
  TArray<FCesiumPropertyTableDescription> PropertyTables;

  /**
   * Descriptions of property textures in the visible glTF models across
   * the tileset.
   */
  UPROPERTY(
      Meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Use PropertyTextures on the CesiumFeaturesMetadataDescription's ModelMetadata instead."))
  TArray<FCesiumPropertyTextureDescription> PropertyTextures;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
  virtual void PostEditChangeChainProperty(
      FPropertyChangedChainEvent& PropertyChangedChainEvent) override;
#endif

protected:
  /** PostLoad override. */
  virtual void PostLoad() override;

private:
  void syncTilesetStatistics();
};
