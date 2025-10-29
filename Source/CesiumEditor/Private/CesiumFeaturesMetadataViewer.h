// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeaturesMetadataDescription.h"
#include "CesiumMetadataValue.h"
#include "CesiumMetadataValueType.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/SWindow.h"

#include <optional>
#include <swl/variant.hpp>
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

  enum class EPropertyQualifiers : uint8 {
    None = 0,
    Offset = 1,
    Scale = 2,
    NoData = 3,
    Default = 4
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
    FCesiumMetadataValueType type;
    int64 arraySize;
    bool isNormalized;
    EPropertySource source;
    TSharedRef<FString> pSourceName;
    uint8 qualifiers; // Bitmask

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
  void gatherGltfPropertyTables(const FCesiumModelMetadata& modelMetadata);

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

  bool canBeRegistered(TSharedRef<StatisticView> pItem);

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
};
