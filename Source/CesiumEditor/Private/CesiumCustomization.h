// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Internationalization/Text.h"
#include "UObject/NameTypes.h"

class IDetailGroup;
class IDetailCategoryBuilder;

class CesiumCustomization {
public:
  /// <summary>
  /// Adds a new group to the given category. This is equivalent to calling
  /// `Category.AddGroup` except that it allows the label to span the entire row
  /// rather than confining it to the "name" column for no apparent reason.
  /// </summary>
  /// <param name="Category">The category to which to add the group.</param>
  /// <param name="GroupName">The name of the group.</param>
  /// <param name="LocalizedDisplayName">The display name of the group.</param>
  /// <param name="bForAdvanced">True if the group should appear in the advanced
  /// section of the category.</param>
  /// <param name="bForAdvanced">True if the group should start
  /// expanded.</param>
  /// <returns>The newly-created group.</returns>
  static IDetailGroup& CreateGroup(
      IDetailCategoryBuilder& Category,
      FName GroupName,
      const FText& LocalizedDisplayName,
      bool bForAdvanced = false,
      bool bStartExpanded = false);
};
