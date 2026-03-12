// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "MetadataInterfaceCommon.h"

FCesiumMetadataEncodingDetails getSelectedEncodingDetails(
    const TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataConversion>>>&
        pConversionCombo,
    const TSharedPtr<SComboBox<TSharedRef<ECesiumEncodedMetadataType>>>&
        pEncodedTypeCombo,
    const TSharedPtr<
        SComboBox<TSharedRef<ECesiumEncodedMetadataComponentType>>>&
        pEncodedComponentTypeCombo) {
  if (!pConversionCombo || !pEncodedTypeCombo || !pEncodedComponentTypeCombo)
    return FCesiumMetadataEncodingDetails();

  TSharedPtr<ECesiumEncodedMetadataConversion> pConversion =
      pConversionCombo->GetSelectedItem();
  TSharedPtr<ECesiumEncodedMetadataType> pEncodedType =
      pEncodedTypeCombo->GetSelectedItem();
  TSharedPtr<ECesiumEncodedMetadataComponentType> pEncodedComponentType =
      pEncodedComponentTypeCombo->GetSelectedItem();

  return FCesiumMetadataEncodingDetails(
      pEncodedType.IsValid() ? *pEncodedType : ECesiumEncodedMetadataType::None,
      pEncodedComponentType.IsValid()
          ? *pEncodedComponentType
          : ECesiumEncodedMetadataComponentType::None,
      pConversion.IsValid() ? *pConversion
                            : ECesiumEncodedMetadataConversion::None);
}

bool validateEncodingDetails(const FCesiumMetadataEncodingDetails& details) {
  switch (details.Conversion) {
  case ECesiumEncodedMetadataConversion::Coerce:
  case ECesiumEncodedMetadataConversion::ParseColorFromString:
    return details.HasValidType();
  case ECesiumEncodedMetadataConversion::None:
  default:
    return false;
  }
}
