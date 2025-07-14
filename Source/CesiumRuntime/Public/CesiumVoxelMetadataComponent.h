// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeaturesMetadataDescription.h"
#include "Templates/UniquePtr.h"

#if WITH_EDITOR
#include "Materials/MaterialFunctionMaterialLayer.h"
#endif

#include "CesiumVoxelMetadataComponent.generated.h"

/**
 * @brief Description of the metadata properties available in the class used by
 * the 3DTILES_content_voxels extension. Exposes what properties are available
 * to use in a custom shader in Unreal materials.
 */
USTRUCT() struct CESIUMRUNTIME_API FCesiumVoxelClassDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The ID of the class in the tileset's metadata schema.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Metadata",
      Meta = (TitleProperty = "Name"))
  FString ID;

  /**
   * @brief Descriptions of properties to pass to the Unreal material.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyAttributePropertyDescription> Properties;
};

/**
 * @brief A component that can be added to Cesium3DTileset actors to
 * view and style metadata embedded in voxels. The properties can be
 * automatically populated by clicking the "Auto Fill" button. Once a selection
 * of desired metadata is made, the boiler-plate material code to access the
 * selected properties and apply custom shaders can be auto-generated using the
 * "Generate Material" button.
 */
UCLASS(ClassGroup = (Cesium), Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumVoxelMetadataComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UCesiumVoxelMetadataComponent();

#if WITH_EDITOR
  /**
   * Populate the description of metadata and feature IDs using the current view
   * of the tileset. This determines what to encode to the GPU based on the
   * existing metadata.
   *
   * Warning: Using Auto Fill may populate the description with a large amount
   * of metadata. Make sure to delete the properties that aren't relevant.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void AutoFill();

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

  /**
   * A preview of the generated custom shader.
   */
  UPROPERTY(
      VisibleAnywhere,
      Transient,
      NonTransactional,
      Category = "Cesium",
      Meta =
          (NoResetToDefault,
           DisplayAfter = "TargetMaterialLayer",
           MultiLine = true))
  FString CustomShaderPreview;

  /**
   * The custom shader code to apply to each voxel that is raymarched.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (TitleProperty = "Custom Shader",
           DisplayAfter = "CustomShaderPreview",
           MultiLine = true))
  FString CustomShader = TEXT("return 1;");

  /**
   * Any additional functions to include for use in the custom shader.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (TitleProperty = "Additional Functions",
           DisplayAfter = "CustomShader",
           MultiLine = true))
  FString AdditionalFunctions;
#endif

  /**
   * A description of the class used by the 3DTILES_content_voxel extension in
   * the tileset.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (TitleProperty = "Voxel Class", DisplayAfter = "AdditionalFunctions"))
  FCesiumVoxelClassDescription Description;

protected:
#if WITH_EDITOR
  virtual void PostLoad() override;

  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
  virtual void PostEditChangeChainProperty(
      FPropertyChangedChainEvent& PropertyChangedChainEvent) override;
#endif

private:
  TObjectPtr<UTexture> pDefaultVolumeTexture;

#if WITH_EDITOR
  static const FString ShaderPreviewTemplate;
  void UpdateShaderPreview();
#endif
};
