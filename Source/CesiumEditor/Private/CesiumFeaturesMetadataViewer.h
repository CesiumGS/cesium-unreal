// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdSet.h"
#include "CesiumMetadataEncodingDetails.h"
#include "CesiumMetadataPropertyDetails.h"
#include "CesiumMetadataValue.h"
#include "CesiumMetadataValueType.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWindow.h"

#include <optional>
#include <vector>

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
  void Sync();

private:
  static TSharedPtr<CesiumFeaturesMetadataViewer> _pExistingWindow;
  TSharedPtr<SVerticalBox> _pContent;

  struct TablePropertyInstanceDetails {
    TArray<TSharedRef<ECesiumEncodedMetadataConversion>> conversionMethods;
    TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataConversion>>>
        pConversionCombo;
    TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataType>>>
        pEncodedTypeCombo;
    TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataComponentType>>>
        pEncodedComponentTypeCombo;
  };

  struct TexturePropertyInstanceDetails {
    bool hasKhrTextureTransform;
  };

  /**
   * A view of an instance of a CesiumGltf::Property for a particular glTF in
   * the tileset. It is technically possible for a tileset to have models with
   * the same property, but different schema definitions. This attempts to
   * capture each different instance so the user can make an informed choice
   * about the behavior.
   */
  struct PropertyInstanceView {
    TSharedRef<FString> pPropertyId;
    FCesiumMetadataPropertyDetails propertyDetails;
    TSharedRef<FString> pSourceName;
    std::variant<TablePropertyInstanceDetails, TexturePropertyInstanceDetails>
        sourceDetails;

    bool operator==(const PropertyInstanceView& rhs) const;
    bool operator!=(const PropertyInstanceView& rhs) const;
  };

  /**
   * A view of a CesiumGltf::Property.
   */
  struct PropertyView {
    TSharedRef<FString> pId;
    TArray<TSharedRef<PropertyInstanceView>> instances;
  };

  /**
   * A view of an instance of a CesiumGltf::FeatureId for a particular glTF in
   * the tileset. Feature IDs in EXT_mesh_features are referenced by index, not
   * by name, so it is technically possible for two feature ID sets with the
   * same index to be differently typed, or to reference different property
   * tables. This attempts to capture each different instance so the user can
   * make an informed choice about the behavior.
   */
  struct FeatureIdSetInstanceView {
    TSharedRef<FString> pFeatureIdSetName;
    ECesiumFeatureIdSetType type;
    bool hasKhrTextureTransform;
    TSharedRef<FString> pPropertyTableName;

    bool operator==(const FeatureIdSetInstanceView& rhs) const;
    bool operator!=(const FeatureIdSetInstanceView& rhs) const;
  };

  /**
   * A view of a CesiumGltf::FeatureId.
   */
  struct FeatureIdSetView {
    TSharedRef<FString> pName;
    TArray<TSharedRef<FeatureIdSetInstanceView>> instances;
  };

  template <
      typename TSource,
      typename TSourceBlueprintLibrary,
      typename TSourceProperty,
      typename TSourcePropertyBlueprintLibrary>
  void gatherGltfPropertySources(const TArray<TSource>& sources);
  void gatherGltfFeaturesMetadata();


  TSharedRef<ITableRow> createPropertyInstanceRow(
      TSharedRef<PropertyInstanceView> pItem,
      const TSharedRef<STableViewBase>& list);
  void createGltfPropertyDropdown(
      TSharedRef<SScrollBox>& pContent,
      const PropertyView& property);

  template <typename TEnum>
  TSharedRef<SWidget> createEnumDropdownOption(TSharedRef<TEnum> pOption);
  template <typename TEnum>
  void createEnumComboBox(
      TSharedPtr<SComboBox<TSharedRef<TEnum>>>& pComboBox,
      const TArray<TSharedRef<TEnum>>& options,
      TEnum initialValue,
      const FString& tooltip);

  TSharedRef<ITableRow> createFeatureIdSetInstanceRow(
      TSharedRef<FeatureIdSetInstanceView> pItem,
      const TSharedRef<STableViewBase>& list);
  void createGltfFeatureIdSetDropdown(
      TSharedRef<SScrollBox>& pContent,
      const FeatureIdSetView& property);

  bool canBeRegistered(TSharedRef<PropertyInstanceView> pItem);
  bool canBeRegistered(TSharedRef<FeatureIdSetInstanceView> pItem);

  void registerPropertyInstance(TSharedRef<PropertyInstanceView> pItem);
  void registerFeatureIdSetInstance(TSharedRef<FeatureIdSetInstanceView> pItem);

  TWeakObjectPtr<ACesium3DTileset> _pTileset;
  TWeakObjectPtr<UCesiumFeaturesMetadataComponent> _pFeaturesMetadataComponent;

  // The current Features / Metadata implementation folds the class / property
  // schemas into each implementation of PropertyTable, PropertyTableProperty,
  // etc., so this functions as a property-centric view instead of a class-based
  // one.
  TArray<PropertyView> _metadataProperties;
  TArray<FeatureIdSetView> _featureIdSets;
  TSet<FString> _propertyTextureNames;

  // Avoid allocating numerous instances of simple enum values (pointers / refs
  // are required for SComboBox).
  TArray<TSharedRef<ECesiumEncodedMetadataConversion>> _conversionOptions;
  TArray<TSharedRef<ECesiumEncodedMetadataType>> _encodedTypeOptions;
  TArray<TSharedRef<ECesiumEncodedMetadataComponentType>>
      _encodedComponentTypeOptions;

  // Lookup map to reduce the number of strings allocated for duplicate property
  // names.
  TMap<FString, TSharedRef<FString>> _stringMap;

  TSharedRef<FString> getSharedRef(const FString& string);
};
