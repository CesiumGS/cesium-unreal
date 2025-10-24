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

class CesiumFeaturesMetadataViewer : public SWindow {
  SLATE_BEGIN_ARGS(CesiumFeaturesMetadataViewer) {}
  /**
   * The target tileset.
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

  typedef swl::variant<
      FCesiumPropertyTablePropertyDescription,
      FCesiumPropertyTexturePropertyDescription>
      PropertyDescription;

  struct StatisticView {
    TSharedRef<FString> pClassId;
    TSharedRef<FString> pPropertyId;
    FString id;
    FCesiumMetadataValue value;
  };

  struct PropertyStatisticsView {
    TSharedRef<FString> pClassId;
    TSharedRef<FString> pId;
    TArray<TSharedRef<StatisticView>> statistics;
  };

  struct ClassStatisticsView {
    TSharedRef<FString> pId;
    TArray<TSharedRef<PropertyStatisticsView>> properties;
  };

  void gatherTilesetStatistics();
  void gatherGltfMetadata();

  TSharedRef<ITableRow> createStatisticRow(
      TSharedRef<StatisticView> item,
      const TSharedRef<STableViewBase>& list);
  // TSharedRef<ITableRow>
  // createPropertyRow(TSharedPtr<PropertyDescription> property);

  TSharedRef<ITableRow> createPropertyStatisticsDropdown(
      TSharedRef<PropertyStatisticsView> pItem,
      const TSharedRef<STableViewBase>& list);
  void createClassStatisticsDropdown(
      TSharedRef<SVerticalBox>& pVertical,
      const ClassStatisticsView& classStatistics);
  void createGltfClassDropdown(TSharedRef<SVerticalBox>& pVertical);

  void registerStatistic(TSharedRef<StatisticView> item);
  // template <typename PropertyType>
  // void registerProperty(TSharedPtr<PropertyType>& property);

  TWeakObjectPtr<ACesium3DTileset> _pTileset;
  TArray<ClassStatisticsView> _statisticsClasses;
};
