// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "AssetData.h"
#include "AssetRegistryModule.h"

#include "CoreMinimal.h"

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

#include <string>
#include <vector>

class ITableRow;
class STableViewBase;

/**
 * A widget that shows a list of assets.
 *
 * Assets can be added by calling `AddAsset`, passing in the
 * "object path" of the assets. They are internally resolved
 * to obtain the `FAssetData`, so that the resulting list
 * entries can be dragged-and-dropped to other target widgets
 * like the viewport or the details view.
 */
class AssetDataList : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(AssetDataList) {}
  SLATE_END_ARGS()

  AssetDataList();
  ~AssetDataList();

  void Construct(const FArguments& Args);

  /**
   * Removes all elements from this list.
   */
  void ClearList();

  /**
   * @brief Add the specified asset to be displayed in this list.
   *
   * The given path is the "object path", as required my the Unreal Asset
   * Registry, in the form `Package.GroupNames.AssetName`.
   *
   * Adding the actual item in the list may be deferred until all assets
   * have been loaded.
   *
   * @param objectPath The object path.
   */
  void AddAsset(const std::string& objectPath);

private:
  /**
   * @brief The actual items that are displayed in the list.
   *
   * These are stored as `FAssetData` objects, as obtained from
   * the Unreal Asset registry, which provide all sorty of
   * (meta) information about an asset, and can be consumed
   * by certain target widgets (like the viewport) during
   * drag-and-drop operations.
   */
  TArray<TSharedPtr<FAssetData>> _items;

  /**
   * @brief The list view that shows the `_items`.
   */
  TSharedPtr<SListView<TSharedPtr<FAssetData>>> _listView;

  /**
   * @brief Object paths of objects that have not been loaded yet.
   *
   * See 'AddAsset' for details.
   */
  std::vector<std::string> _pendingObjectPaths;

  /**
   * @brief Internal method to actually add the item to the list.
   *
   * This will be called after all assets have been loaded and
   * the `FAssetData` for the specified object can be resolved.
   *
   * @param objectPath The object path.
   */
  void _AddAssetInternal(const std::string& objectPath);

  /**
   * @brief Callback to create a row in the list view.
   *
   * See Unreal Slate documentation...
   */
  TSharedRef<ITableRow> _CreateRow(
      TSharedPtr<FAssetData> item,
      const TSharedRef<STableViewBase>& list);

  /**
   * @brief Will be called when a drag-and-drop of a list item starts.
   */
  FReply
  _OnDragging(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

  /**
   * @brief Will be called after all assets have been loaded in
   * the asset registry.
   *
   * This will obtain the `FAssetData` for all `_pendingObjectPaths`,
   * and populate the list with the corresponding items.
   */
  void _HandleFilesLoaded();
};
