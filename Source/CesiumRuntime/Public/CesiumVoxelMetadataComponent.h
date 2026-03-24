// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeaturesMetadataDescription.h"
#include "CesiumMetadataComponent.h"
#include "Templates/UniquePtr.h"

#if WITH_EDITOR
#include "Materials/MaterialFunctionMaterialLayer.h"
#endif

#include "CesiumVoxelMetadataComponent.generated.h"

class UTexture;

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
  UPROPERTY(EditAnywhere, Category = "Cesium|Metadata")
  FString ClassId;

  /**
   * @brief Descriptions of properties to pass to the Unreal material.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyAttributePropertyDescription> Properties;

  /**
   * @brief Description of the statistics of the properties on the voxel class.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Metadata|Statistics",
      Meta = (TitleProperty = "Id"))
  TArray<FCesiumMetadataPropertyStatisticsDescription> Statistics;
};

/**
 * @brief A component that can be added to Cesium3DTileset actors to
 * view and style metadata embedded in voxel content. "Build Shader" allows
 * users to view the voxel metadata properties available in the tileset and
 * write custom code to style them. Then, the boiler-plate material code to
 * access the selected properties and apply custom shaders can be auto-generated
 * using the "Generate Material" button.
 */
UCLASS(ClassGroup = (Cesium), Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumVoxelMetadataComponent
    : public UCesiumMetadataComponent {
  GENERATED_BODY()

public:
  UCesiumVoxelMetadataComponent();

#if WITH_EDITOR
  /**
   * Opens a window to build a custom shader for the tileset using its available
   * voxel properties.
   */
  UFUNCTION(
      CallInEditor,
      Category = "Cesium",
      Meta = (DisplayName = "Build Shader"))
  void BuildShader();

  /**
   * Creates or overwrites a boiler-plate material that exposes the requested
   * metadata properties in the current description and writes out the custom
   * shader. The changes will be applied to TargetMaterial if it is already
   * specified. Otherwise a new material layer will be created in the /Content/
   * folder and TargetMaterial will be set to the new material.
   */
  UFUNCTION(
      CallInEditor,
      Category = "Cesium",
      Meta = (DisplayName = "Generate Material"))
  void GenerateMaterial();
#endif

#if WITH_EDITORONLY_DATA
  /**
   * This is the target UMaterial that the boiler-plate material generation will
   * use. When pressing "Generate Material", nodes will be added to this
   * material to enable access to the requested metadata. If this is left blank,
   * a new material will be created in the /Content/ folder.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterial* TargetMaterial = nullptr;

  /**
   * The custom shader code to apply to each voxel that is raymarched.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (TitleProperty = "Custom Shader",
           DisplayAfter = "TargetMaterialLayer",
           MultiLine = true))
  FString CustomShader = TEXT("return float4(1, 1, 1, 0.02);");

  /**
   * Any additional functions to include for use in the custom shader. The HLSL
   * code provided here is included verbatim in the generated material.
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

  /**
   * Gets a preview of the generated custom shader.
   */
  FString getCustomShaderPreview() const;

protected:
  virtual void OnFetchMetadata(
      ACesium3DTileset* pActor,
      const Cesium3DTilesSelection::TilesetMetadata* pMetadata);
  virtual void ClearStatistics();

private:
  TObjectPtr<UTexture> _pDefaultVolumeTexture;
#if WITH_EDITOR
  static const FString _shaderPreviewTemplate;
#endif
};
