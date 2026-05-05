// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeaturesMetadataDescription.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"

#include <optional>

/**
 * A view of a statistic value in 3D Tiles metadata.
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

FCesiumMetadataEncodingDetails getSelectedEncodingDetails(
    const TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataConversion>>>&
        pConversionCombo,
    const TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataType>>>&
        pEncodedTypeCombo,
    const TSharedPtr<
        SComboBox<TSharedRef<ECesiumEncodedMetadataComponentType>>>&
        pEncodedComponentTypeCombo);

bool validateEncodingDetails(const FCesiumMetadataEncodingDetails& details);

/**
 * An enum describing whether a property matches an existing one on the target
 * UMetadataComponent.
 */
enum class ComponentSearchResult { NoMatch, PartialMatch, ExactMatch };

/**
 * Utility for interacting with enums to more easily build user interfaces.
 */
template <typename TEnum> struct MetadataEnumUtility {
  // Avoid allocating numerous instances of simple enum values (because shared
  // pointers /refs are required for SComboBox).
  TArray<TSharedRef<TEnum>> options;

  MetadataEnumUtility();
  TArray<TSharedRef<TEnum>> getSharedRefs(const TArray<TEnum>& selection);

  static FString enumToNameString(TEnum value);
  static FText getEnumDisplayNameText(TEnum value);
};

template <typename TEnum>
MetadataEnumUtility<TEnum>::MetadataEnumUtility() : options() {
  UEnum* pEnum = StaticEnum<TEnum>();
  if (pEnum) {
    // "NumEnums" also includes the "_MAX" value, which indicates the number of
    // different values in the enum. Exclude it here.
    const int32 num = pEnum->NumEnums() - 1;
    options.Reserve(num);

    for (int32 i = 0; i < num; i++) {
      TEnum value = TEnum(pEnum->GetValueByIndex(i));
      options.Emplace(MakeShared<TEnum>(value));
    }
  }
}

template <typename TEnum>
/*static*/ FString MetadataEnumUtility<TEnum>::enumToNameString(TEnum value) {
  const UEnum* pEnum = StaticEnum<TEnum>();
  return pEnum ? pEnum->GetNameStringByValue((int64)value) : FString();
}

template <typename TEnum>
/*static*/ FText
MetadataEnumUtility<TEnum>::getEnumDisplayNameText(TEnum value) {
  UEnum* pEnum = StaticEnum<TEnum>();
  return pEnum ? pEnum->GetDisplayNameTextByValue(int64(value))
               : FText::FromString(FString());
}

template <typename TEnum>
/*static*/ TArray<TSharedRef<TEnum>>
MetadataEnumUtility<TEnum>::getSharedRefs(const TArray<TEnum>& selection) {
  TArray<TSharedRef<TEnum>> result;
  UEnum* pEnum = StaticEnum<TEnum>();
  if (!pEnum) {
    return result;
  }

  // Assumes options will be initialized in enum order!
  for (TEnum value : selection) {
    int32 index = pEnum->GetIndexByValue(int64(value));
    CESIUM_ASSERT(index >= 0 && index < options.Num());
    result.Add(options[index]);
  }

  return result;
}

static MetadataEnumUtility<ECesiumEncodedMetadataConversion> ConversionEnum;
static MetadataEnumUtility<ECesiumEncodedMetadataType> EncodedTypeEnum;
static MetadataEnumUtility<ECesiumEncodedMetadataComponentType>
    EncodedComponentTypeEnum;
