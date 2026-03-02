// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumVoxelMetadataComponent.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "MetadataCommon.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWindow.h"

#include <optional>

class SHorizontalBox;
class SMultiLineEditableTextBox;

class ACesium3DTileset;
class UCesiumVoxelMetadataComponent;
class FHLSLSyntaxHighlighterMarshaller;

class CesiumVoxelShaderBuilder : public SWindow {
  SLATE_BEGIN_ARGS(CesiumVoxelShaderBuilder) {}
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
  struct VoxelPropertyView {
    /**
     * The ID of the property.
     */
    TSharedRef<FString> pId;

    /**
     * The type and other details of this property.
     */
    FCesiumMetadataPropertyDetails propertyDetails;

    /**
     * Additional details encoding this property.
     */
    PropertyInstanceEncodingDetails encodingDetails;

    /**
     * The statistics of the property.
     */
    TArray<TSharedRef<StatisticView>> statistics;

    // bool operator==(const PropertyInstance& rhs) const;
    // bool operator!=(const PropertyInstance& rhs) const;
  };

  struct VoxelClassView {
    /**
     * The ID of the voxel class.
     */
    TSharedRef<FString> pId;

    /**
     * The properties belonging to the voxel class.
     */
    TArray<TSharedRef<VoxelPropertyView>> properties;
  };

  void gatherVoxelPropertiesAndStatistics();

  /**
   * @brief Syncs with any property encoding details present on the
   * UCesiumVoxelMetadataComponent.
   */
  void syncPropertyEncodingDetails();

  TSharedRef<ITableRow> createStatisticRow(
      TSharedRef<StatisticView> pItem,
      const TSharedRef<STableViewBase>& list);
  TSharedRef<ITableRow> createVoxelPropertyDropdown(
      TSharedRef<VoxelPropertyView> pItem,
      const TSharedRef<STableViewBase>& list);
  void createVoxelClassDropdown();

  // enum ComponentSearchResult { NoMatch, PartialMatch, ExactMatch };

  // ComponentSearchResult findOnComponent(TSharedRef<StatisticView> pItem)
  // const; ComponentSearchResult findOnComponent(
  //     TSharedRef<PropertyInstance> pItem,
  //     bool compareEncodingDetails) const;

  void registerStatistic(TSharedRef<StatisticView> pItem);
  // void registerPropertyInstance(TSharedRef<PropertyInstance> pItem);

  void removeStatistic(TSharedRef<StatisticView> pItem);
  // void removePropertyInstance(TSharedRef<PropertyInstance> pItem);

  static TSharedRef<FString> getSharedRef(const FString& string);

  static TSharedPtr<CesiumVoxelShaderBuilder> _pExistingWindow;
  TSharedPtr<SScrollBox> _pVoxelClassContent;

  TWeakObjectPtr<ACesium3DTileset> _pTileset;
  TWeakObjectPtr<UCesiumVoxelMetadataComponent> _pVoxelMetadataComponent;

  TUniquePtr<VoxelClassView> _pVoxelClass;
  FString _customShaderPreview;

  // Lookup map to reduce the number of strings allocated for duplicate property
  // names.
  static TMap<FString, TSharedRef<FString>> _stringMap;

  // Shader text box, inspired by SGraphNodeMaterialCustom.h
  TSharedPtr<FHLSLSyntaxHighlighterMarshaller> _pSyntaxHighlighter;
  TSharedPtr<SMultiLineEditableTextBox> _pShaderPreview;
  TSharedPtr<SMultiLineEditableTextBox> _pCustomShader;
  TSharedPtr<SMultiLineEditableTextBox> _pAdditionalFunctions;
};
