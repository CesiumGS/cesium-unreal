// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeaturesMetadataDescription.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"

#include <optional>

/**
 * A view of a statistic value.
 */
struct StatisticView {
  /**
   * The ID of the class to which the property for this statistic belongs.
   */
  TSharedRef<FString> pClassId;
  /**
   * The ID of the property to which this statistic applies.
   */
  TSharedRef<FString> pPropertyId;
  /**
   * The semantic of this statistic.
   */
  ECesiumMetadataStatisticSemantic semantic;
  /**
   * The value of this statistic.
   */
  FCesiumMetadataValue value;
};

/**
 * A view of an instance of Cesium3DTileset::PropertyStatistics.
 */
struct PropertyStatisticsView {
  /**
   * The ID of the class to which the property belongs.
   */
  TSharedRef<FString> pClassId;
  /**
   * The ID of the property to which these statistics apply.
   */
  TSharedRef<FString> pId;
  /**
   * The statistics of the property.
   */
  TArray<TSharedRef<StatisticView>> statistics;
};

/**
 * A view of an instance of Cesium3DTileset::ClassStatistics.
 */
struct ClassStatisticsView {
  /**
   * The ID of the class to which these statistics apply.
   */
  TSharedRef<FString> pId;
  /**
   * The properties belonging to the class.
   */
  TArray<TSharedRef<PropertyStatisticsView>> properties;
};

/**
 * Encoding details for a `CesiumGltf::PropertyTableProperty` or
 * `CesiumGltf::PropertyAttributeProperty` instance.
 */
struct PropertyInstanceEncodingDetails {
  /**
   * The possible conversion methods for this property. Contains a subset of
   * the values in ECesiumEncodedMetadataConversion.
   */
  TArray<TSharedRef<ECesiumEncodedMetadataConversion>> conversionMethods;
  /**
   * The combo box widget for selecting the conversion method.
   */
  TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataConversion>>>
      pConversionCombo;
  /**
   * The option to set as the selected conversion, if previously specified
   * (e.g., by details queried from UCesiumFeatureMetadataComponent).
   */
  TSharedPtr<ECesiumEncodedMetadataConversion> pConversionSelection;
  /**
   * The combo box widget for selecting the encoded type.
   */
  TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataType>>>
      pEncodedTypeCombo;
  /**
   * The option to set as the selected encoded type, if previously specified
   * (e.g., by details queried from UCesiumFeatureMetadataComponent).
   */
  TSharedPtr<ECesiumEncodedMetadataType> pEncodedTypeSelection;
  /**
   * The combo box widget for selecting the encoded component type.
   */
  TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataComponentType>>>
      pEncodedComponentTypeCombo;
  /**
   * The option to set as the selected encoded component type, if previously
   * specified (e.g., by details queried from
   * UCesiumFeatureMetadataComponent).
   */
  TSharedPtr<ECesiumEncodedMetadataComponentType>
      pEncodedComponentTypeSelection;
};

/**
 * An instance of a CesiumGltf::PropertyTableProperty or
 * CesiumGltf::PropertyTextureProperty for a particular glTF in the tileset.
 *
 * It is technically possible for a tileset to have models with the same
 * property, but different schema definitions. This attempts to capture each
 * different instance so the user can make an informed choice about the
 * behavior.
 */
struct PropertyInstance {
  /**
   * The ID of the property of which this is an instance.
   */
  TSharedRef<FString> pPropertyId;
  /**
   * The type and other details of this property instance.
   */
  FCesiumMetadataPropertyDetails propertyDetails;
  /**
   * The name of the source with which this property is associated.
   */
  TSharedRef<FString> pSourceName;
  /**
   * Additional details encoding this property instance. Only used for
   * `CesiumGltf::PropertyTableProperty`.
   */
  std::optional<PropertyInstanceEncodingDetails> encodingDetails;

  bool operator==(const PropertyInstance& rhs) const;
  bool operator!=(const PropertyInstance& rhs) const;
};

/**
 * A view of a class property in EXT_structural_metadata.
 */
struct PropertyView {
  /**
   * The ID of the property.
   */
  TSharedRef<FString> pId;
  /**
   * The instances of this property.
   */
  TArray<TSharedRef<PropertyInstance>> instances;
};

enum EPropertySource { PropertyTable, PropertyTexture, PropertyAttribute };

/**
 * A view of either a CesiumGltf::PropertyTable or
 * CesiumGltf::PropertyTexture.
 */
struct PropertySourceView {
  /**
   * The name generated for this property source.
   */
  TSharedRef<FString> pName;
  /**
   * The type of this source.
   */
  EPropertySource type;
  /**
   * The properties belonging to this source.
   */
  TArray<TSharedRef<PropertyView>> properties;
};

// Avoid allocating numerous instances of simple enum values (because shared
// pointers /refs are required for SComboBox).

struct MetadataUI {
  static TArray<TSharedRef<ECesiumEncodedMetadataConversion>> conversionOptions;
  static TArray<TSharedRef<ECesiumEncodedMetadataType>> encodedTypeOptions;
  static TArray<TSharedRef<ECesiumEncodedMetadataComponentType>>
      encodedComponentTypeOptions;
};

static MetadataUI ui;
