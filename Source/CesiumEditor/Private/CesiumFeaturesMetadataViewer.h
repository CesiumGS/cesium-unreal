// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdSet.h"
#include "CesiumMetadataEncodingDetails.h"
#include "CesiumMetadataPropertyDetails.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWindow.h"

#include <optional>

class ACesium3DTileset;
class UCesiumFeaturesMetadataComponent;
struct FCesiumModelMetadata;

class CesiumFeaturesMetadataViewer : public SWindow {
  SLATE_BEGIN_ARGS(CesiumFeaturesMetadataViewer) {}
  /**
   * The tileset that is being queried for features and metadata.
   */
  SLATE_ARGUMENT(TWeakObjectPtr<ACesium3DTileset>, Tileset)
  SLATE_END_ARGS()

public:
  static void Open(TWeakObjectPtr<ACesium3DTileset> pTileset);
  void Construct(const FArguments& InArgs);

  /**
   * Syncs the window's UI with the current view of the tileset.
   */
  void SyncAndRebuildUI();

private:
  /**
   * Encoding details for a `CesiumGltf::PropertyTableProperty` instance.
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

  enum EPropertySource { PropertyTable, PropertyTexture };

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

  /**
   * A view of an instance of a CesiumGltf::FeatureId for a particular glTF in
   * the tileset. Feature IDs in EXT_mesh_features are referenced by index, not
   * by name, so it is technically possible for two feature ID sets with the
   * same index to be differently typed, or to reference different property
   * tables. This attempts to capture each different instance so the user can
   * make an informed choice about the behavior.
   */
  struct FeatureIdSetInstance {
    /**
     * The name of the feature ID set for which this is an instance.
     */
    TSharedRef<FString> pFeatureIdSetName;
    /**
     * The type of this instance.
     */
    ECesiumFeatureIdSetType type;
    /**
     * The name of the property table that this instance references.
     */
    TSharedRef<FString> pPropertyTableName;

    bool operator==(const FeatureIdSetInstance& rhs) const;
    bool operator!=(const FeatureIdSetInstance& rhs) const;
  };

  /**
   * A view of a CesiumGltf::FeatureId.
   */
  struct FeatureIdSetView {
    /**
     * The name generated for this feature ID set.
     */
    TSharedRef<FString> pName;
    /**
     * The instances of this feature ID set.
     */
    TArray<TSharedRef<FeatureIdSetInstance>> instances;
  };

  template <
      typename TSource,
      typename TSourceBlueprintLibrary,
      typename TSourceProperty,
      typename TSourcePropertyBlueprintLibrary>
  void gatherGltfPropertySources(const TArray<TSource>& sources);
  void gatherGltfFeaturesMetadata();

  /**
   * @brief Syncs with any property encoding details present on the
   * UCesiumFeaturesMetadataComponent.
   */
  void syncPropertyEncodingDetails();

  TSharedRef<ITableRow> createPropertyInstanceRow(
      TSharedRef<PropertyInstance> pItem,
      const TSharedRef<STableViewBase>& list);
  TSharedRef<ITableRow> createGltfPropertyDropdown(
      TSharedRef<PropertyView> pItem,
      const TSharedRef<STableViewBase>& list);
  void createGltfPropertySourceDropdown(
      TSharedRef<SScrollBox>& pContent,
      const PropertySourceView& source);

  template <typename TEnum>
  TSharedRef<SWidget> createEnumDropdownOption(TSharedRef<TEnum> pOption);
  template <typename TEnum>
  void createEnumComboBox(
      TSharedPtr<SComboBox<TSharedRef<TEnum>>>& pComboBox,
      const TArray<TSharedRef<TEnum>>& options,
      TEnum initialValue,
      const FString& tooltip);

  TSharedRef<ITableRow> createFeatureIdSetInstanceRow(
      TSharedRef<FeatureIdSetInstance> pItem,
      const TSharedRef<STableViewBase>& list);
  void createGltfFeatureIdSetDropdown(
      TSharedRef<SScrollBox>& pContent,
      const FeatureIdSetView& property);

  enum ComponentSearchResult { NoMatch, PartialMatch, ExactMatch };

  ComponentSearchResult findOnComponent(
      TSharedRef<PropertyInstance> pItem,
      bool compareEncodingDetails);
  ComponentSearchResult findOnComponent(TSharedRef<FeatureIdSetInstance> pItem);

  void registerPropertyInstance(TSharedRef<PropertyInstance> pItem);
  void registerFeatureIdSetInstance(TSharedRef<FeatureIdSetInstance> pItem);

  void removePropertyInstance(TSharedRef<PropertyInstance> pItem);
  void removeFeatureIdSetInstance(TSharedRef<FeatureIdSetInstance> pItem);

  static TSharedRef<FString> getSharedRef(const FString& string);
  static void initializeStaticVariables();

  static TSharedPtr<CesiumFeaturesMetadataViewer> _pExistingWindow;
  TSharedPtr<SVerticalBox> _pContent;

  TWeakObjectPtr<ACesium3DTileset> _pTileset;
  TWeakObjectPtr<UCesiumFeaturesMetadataComponent> _pFeaturesMetadataComponent;

  // The current Features / Metadata implementation folds the class / property
  // schemas into each implementation of PropertyTable, PropertyTableProperty,
  // etc. So this functions as a property source-centric view instead of a
  // class-based one.
  TArray<PropertySourceView> _metadataSources;
  TArray<FeatureIdSetView> _featureIdSets;
  TSet<FString> _propertyTextureNames;

  // Avoid allocating numerous instances of simple enum values (because shared
  // pointers /refs are required for SComboBox).
  static TArray<TSharedRef<ECesiumEncodedMetadataConversion>>
      _conversionOptions;
  static TArray<TSharedRef<ECesiumEncodedMetadataType>> _encodedTypeOptions;
  static TArray<TSharedRef<ECesiumEncodedMetadataComponentType>>
      _encodedComponentTypeOptions;
  // Lookup map to reduce the number of strings allocated for duplicate property
  // names.
  static TMap<FString, TSharedRef<FString>> _stringMap;
};
