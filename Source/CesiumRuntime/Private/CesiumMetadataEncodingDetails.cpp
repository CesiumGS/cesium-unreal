// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumMetadataEncodingDetails.h"
#include "CesiumMetadataPropertyDetails.h"

ECesiumEncodedMetadataComponentType CesiumMetadataComponentTypeToEncodingType(
    ECesiumMetadataComponentType ComponentType) {
  switch (ComponentType) {
  case ECesiumMetadataComponentType::Int8: // lossy or reinterpreted
  case ECesiumMetadataComponentType::Uint8:
    return ECesiumEncodedMetadataComponentType::Uint8;
  case ECesiumMetadataComponentType::Int16:
  case ECesiumMetadataComponentType::Uint16:
  case ECesiumMetadataComponentType::Int32:  // lossy or reinterpreted
  case ECesiumMetadataComponentType::Uint32: // lossy or reinterpreted
  case ECesiumMetadataComponentType::Int64:  // lossy
  case ECesiumMetadataComponentType::Uint64: // lossy
  case ECesiumMetadataComponentType::Float32:
  case ECesiumMetadataComponentType::Float64: // lossy
    return ECesiumEncodedMetadataComponentType::Float;
  default:
    return ECesiumEncodedMetadataComponentType::None;
  }
}

ECesiumEncodedMetadataType
CesiumMetadataTypeToEncodingType(ECesiumMetadataType Type) {
  switch (Type) {
  case ECesiumMetadataType::Scalar:
    return ECesiumEncodedMetadataType::Scalar;
  case ECesiumMetadataType::Vec2:
    return ECesiumEncodedMetadataType::Vec2;
  case ECesiumMetadataType::Vec3:
    return ECesiumEncodedMetadataType::Vec3;
  case ECesiumMetadataType::Vec4:
    return ECesiumEncodedMetadataType::Vec4;
  default:
    return ECesiumEncodedMetadataType::None;
  }
}

size_t
CesiumGetEncodedMetadataTypeComponentCount(ECesiumEncodedMetadataType Type) {
  switch (Type) {
  case ECesiumEncodedMetadataType::Scalar:
    return 1;
  case ECesiumEncodedMetadataType::Vec2:
    return 2;
  case ECesiumEncodedMetadataType::Vec3:
    return 3;
  case ECesiumEncodedMetadataType::Vec4:
    return 4;
  default:
    return 0;
  }
}

FCesiumMetadataEncodingDetails::FCesiumMetadataEncodingDetails()
    : Type(ECesiumEncodedMetadataType::None),
      ComponentType(ECesiumEncodedMetadataComponentType::None),
      Conversion(ECesiumEncodedMetadataConversion::None) {}

FCesiumMetadataEncodingDetails::FCesiumMetadataEncodingDetails(
    ECesiumEncodedMetadataType InType,
    ECesiumEncodedMetadataComponentType InComponentType,
    ECesiumEncodedMetadataConversion InConversion)
    : Type(InType), ComponentType(InComponentType), Conversion(InConversion) {}

bool FCesiumMetadataEncodingDetails::operator==(
    const FCesiumMetadataEncodingDetails& Info) const {
  return Type == Info.Type && ComponentType == Info.ComponentType &&
         Conversion == Info.Conversion;
}

bool FCesiumMetadataEncodingDetails::operator!=(
    const FCesiumMetadataEncodingDetails& Info) const {
  return !operator==(Info);
}

bool FCesiumMetadataEncodingDetails::HasValidType() const {
  return Type != ECesiumEncodedMetadataType::None &&
         ComponentType != ECesiumEncodedMetadataComponentType::None;
}

namespace {
ECesiumEncodedMetadataType
getBestFittingEncodedType(FCesiumMetadataPropertyDetails PropertyDetails) {
  ECesiumMetadataType type = PropertyDetails.Type;
  if (PropertyDetails.bIsArray) {
    if (PropertyDetails.ArraySize <= 0) {
      // Variable-length array properties are unsupported.
      return ECesiumEncodedMetadataType::None;
    }

    if (type != ECesiumMetadataType::Boolean &&
        type != ECesiumMetadataType::Scalar) {
      // Only boolean and scalar array properties are supported.
      return ECesiumEncodedMetadataType::None;
    }

    int64 componentCount =
        std::min(PropertyDetails.ArraySize, static_cast<int64>(4));
    switch (componentCount) {
    case 1:
      return ECesiumEncodedMetadataType::Scalar;
    case 2:
      return ECesiumEncodedMetadataType::Vec2;
    case 3:
      return ECesiumEncodedMetadataType::Vec3;
    case 4:
      return ECesiumEncodedMetadataType::Vec4;
    default:
      return ECesiumEncodedMetadataType::None;
    }
  }

  switch (type) {
  case ECesiumMetadataType::Boolean:
  case ECesiumMetadataType::Scalar:
    return ECesiumEncodedMetadataType::Scalar;
  case ECesiumMetadataType::Vec2:
    return ECesiumEncodedMetadataType::Vec2;
  case ECesiumMetadataType::Vec3:
    return ECesiumEncodedMetadataType::Vec3;
  case ECesiumMetadataType::Vec4:
    return ECesiumEncodedMetadataType::Vec4;
  default:
    return ECesiumEncodedMetadataType::None;
  }
}
} // namespace


/*static*/
FCesiumMetadataEncodingDetails
FCesiumMetadataEncodingDetails::GetBestFitForProperty(
    const FCesiumMetadataPropertyDetails& PropertyDetails) {
  ECesiumEncodedMetadataType type = getBestFittingEncodedType(PropertyDetails);

  if (type == ECesiumEncodedMetadataType::None) {
    // The type cannot be encoded at all; return.
    return FCesiumMetadataEncodingDetails();
  }

  ECesiumEncodedMetadataComponentType componentType =
      CesiumMetadataComponentTypeToEncodingType(PropertyDetails.ComponentType);

  return FCesiumMetadataEncodingDetails(
      type,
      componentType,
      ECesiumEncodedMetadataConversion::Coerce);
}
