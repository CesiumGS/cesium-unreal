#include "CesiumEncodedMetadataConversions.h"

ECesiumEncodedMetadataComponentType CesiumMetadataComponentTypeToEncodedType(
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
