// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeaturesMetadataDescription.h"
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

  struct StatisticView {
    TSharedRef<FString> pClassId;
    TSharedRef<FString> pPropertyId;
    ECesiumMetadataStatisticSemantic semantic;
    FCesiumMetadataValue value;
  };

  /**
   * A view of an instance of Cesium3DTileset::PropertyStatistics.
   */
  struct PropertyStatisticsView {
    TSharedRef<FString> pClassId;
    TSharedRef<FString> pId;
    TArray<TSharedRef<StatisticView>> statistics;
  };

  /**
   * A view of an instance of Cesium3DTileset::ClassStatistics.
   */
  struct ClassStatisticsView {
    TSharedRef<FString> pId;
    TArray<TSharedRef<PropertyStatisticsView>> properties;
  };

  enum EPropertySource { PropertyTable = 0, PropertyTexture = 1 };

  /**
   * A view of an instance of a CesiumGltf::Property for a particular model in a
   * tileset. It is technically possible for a tileset to have models with the
   * same property, but different schema definitions. This attempts to capture
   * each different instance so the user can make an informed choice about the
   * behavior.
   */
  struct PropertyInstanceView {
    TSharedRef<FString> pPropertyId;
    FCesiumMetadataPropertyDetails propertyDetails;
    EPropertySource source;
    TSharedRef<FString> pSourceName;
    TArray<TSharedRef<ECesiumEncodedMetadataConversion>> conversionMethods;
    TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataConversion>>>
        pConversionCombo;
    TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataType>>>
        pEncodedTypeCombo;
    TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataComponentType>>>
        pEncodedComponentTypeCombo;

    bool operator==(const PropertyInstanceView& property) const;
    bool operator!=(const PropertyInstanceView& property) const;
  };

  /**
   * A view of an instance of a CesiumGltf::Property.
   */
  struct PropertyView {
    TSharedRef<FString> pId;
    TArray<TSharedRef<PropertyInstanceView>> instances;
  };

  void gatherTilesetStatistics();
  void gatherGltfMetadata();

  template <
      typename TSource,
      typename TSourceBlueprintLibrary,
      typename TSourceProperty,
      typename TSourcePropertyBlueprintLibrary>
  void gatherGltfPropertySources(const TArray<TSource>& sources);

  TSharedRef<ITableRow> createStatisticRow(
      TSharedRef<StatisticView> pItem,
      const TSharedRef<STableViewBase>& list);
  TSharedRef<ITableRow> createPropertyStatisticsDropdown(
      TSharedRef<PropertyStatisticsView> pItem,
      const TSharedRef<STableViewBase>& list);
  void createClassStatisticsDropdown(
      TSharedRef<SScrollBox>& pContent,
      const ClassStatisticsView& classStatistics);

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
      const TArray<TSharedRef<TEnum>>& options);

  bool canBeRegistered(TSharedRef<StatisticView> pItem);
  bool canBeRegistered(TSharedRef<PropertyInstanceView> pItem);

  void registerStatistic(TSharedRef<StatisticView> pItem);
  void registerPropertyInstance(TSharedRef<PropertyInstanceView> pItem);

  TWeakObjectPtr<ACesium3DTileset> _pTileset;
  TWeakObjectPtr<UCesiumFeaturesMetadataComponent> _pFeaturesMetadataComponent;

  TArray<ClassStatisticsView> _statisticsClasses;
  // The current Features / Metadata implementation folds the class / property
  // schemas into each implementation of PropertyTable, PropertyTableProperty,
  // etc., so this functions as a property-centric view instead of a class-based
  // one.
  TArray<PropertyView> _metadataProperties;

  // Avoid allocating numerous instances of simple enum values (pointers / refs
  // are required for SComboBox).
  TArray<TSharedRef<ECesiumEncodedMetadataConversion>> _conversionOptions;
  TArray<TSharedRef<ECesiumEncodedMetadataType>> _encodedTypeOptions;
  TArray<TSharedRef<ECesiumEncodedMetadataComponentType>>
      _encodedComponentTypeOptions;
};
