// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
CesiuMetadataTrueTypeToBlueprintType(ECesiumMetadataTrueType trueType) {
  switch (trueType) {
  case ECesiumMetadataTrueType::Boolean:
    return ECesiumMetadataBlueprintType::Boolean;
  case ECesiumMetadataTrueType::Uint8:
    return ECesiumMetadataBlueprintType::Byte;
  case ECesiumMetadataTrueType::Int8:
  case ECesiumMetadataTrueType::Int16:
  case ECesiumMetadataTrueType::Uint16:
  case ECesiumMetadataTrueType::Int32:
    return ECesiumMetadataBlueprintType::Integer;
  case ECesiumMetadataTrueType::Int64:
    return ECesiumMetadataBlueprintType::Integer64;
  case ECesiumMetadataTrueType::Float32:
    return ECesiumMetadataBlueprintType::Float;
  case ECesiumMetadataTrueType::Uint64:
  case ECesiumMetadataTrueType::Float64:
  case ECesiumMetadataTrueType::String:
    return ECesiumMetadataBlueprintType::String;
  case ECesiumMetadataTrueType::Array:
    return ECesiumMetadataBlueprintType::Array;
  default:
    return ECesiumMetadataBlueprintType::None;
  }
}
