// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumEncodedMetadataConversions.h"
#include "CesiumFeaturesMetadataComponent.h"
#include "CesiumMetadataEncodingDetails.h"
#include "CesiumMetadataPropertyDetails.h"
#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "CesiumPropertyTableProperty.h"
#include <CesiumGltf/MetadataConversions.h>

#include <algorithm>
#include <stdexcept>

namespace {
ECesiumEncodedMetadataType
GetBestFittingEncodedType(FCesiumMetadataPropertyDetails PropertyDetails) {
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

FCesiumMetadataEncodingDetails CesiumMetadataPropertyDetailsToEncodingDetails(
    FCesiumMetadataPropertyDetails PropertyDetails) {
  ECesiumEncodedMetadataType type = GetBestFittingEncodedType(PropertyDetails);

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

namespace {
template <typename T>
void coerceAndEncodeArrays(
    const FCesiumPropertyTablePropertyDescription& propertyDescription,
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData,
    size_t pixelSize) {
  int64 propertySize =
      UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(property);
  int64 arraySize =
      UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(property);
  size_t componentCount = CesiumGetEncodedMetadataTypeComponentCount(
      propertyDescription.EncodingDetails.Type);
  // Encode up to four array elements.
  int64 elementCount = std::min(static_cast<int64>(componentCount), arraySize);

  if (textureData.size() < propertySize * elementCount * sizeof(T)) {
    throw std::runtime_error(
        "Buffer is too small to store the data of this property.");
  }

  uint8* pWritePos = reinterpret_cast<uint8*>(textureData.data());
  for (int64 i = 0; i < propertySize; ++i) {
    FCesiumPropertyArray array =
        UCesiumPropertyTablePropertyBlueprintLibrary::GetArray(property, i);
    if constexpr (std::is_same_v<T, uint8>) {
      for (int64 j = 0; j < elementCount; ++j) {
        const FCesiumMetadataValue& value =
            UCesiumPropertyArrayBlueprintLibrary::GetValue(array, j);
        *(pWritePos + j) =
            UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0);
      }
    } else if constexpr (std::is_same_v<T, float>) {
      // Floats are encoded backwards (e.g., ABGR)
      float* pWritePosF = reinterpret_cast<float*>(pWritePos + pixelSize) - 1;
      for (int64 j = 0; j < elementCount; ++j) {
        const FCesiumMetadataValue& value =
            UCesiumPropertyArrayBlueprintLibrary::GetValue(array, j);
        *pWritePosF =
            UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0.0f);
        --pWritePosF;
      }
    }
    pWritePos += pixelSize;
  }
}

template <typename T>
void coerceAndEncodeScalars(
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData) {
  int64 propertySize =
      UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(property);
  if (textureData.size() < propertySize * sizeof(T)) {
    throw std::runtime_error(
        "Buffer is too small to store the data of this property.");
  }

  T* pWritePos = reinterpret_cast<T*>(textureData.data());

  for (int64 i = 0; i < propertySize; ++i) {
    FCesiumMetadataValue value =
        UCesiumPropertyTablePropertyBlueprintLibrary::GetRawValue(property, i);

    if constexpr (std::is_same_v<T, uint8>) {
      *pWritePos = UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0);
    } else if constexpr (std::is_same_v<T, float>) {
      *pWritePos = UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0.0f);
    }

    ++pWritePos;
  }
  return;
}

template <typename T>
void coerceAndEncodeVec2s(
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData,
    size_t pixelSize) {
  int64 propertySize =
      UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(property);
  if (textureData.size() < propertySize * 2 * sizeof(T)) {
    throw std::runtime_error(
        "Buffer is too small to store the data of this property.");
  }

  uint8* pWritePos = reinterpret_cast<uint8*>(textureData.data());

  for (int64 i = 0; i < propertySize; ++i) {
    FCesiumMetadataValue value =
        UCesiumPropertyTablePropertyBlueprintLibrary::GetRawValue(property, i);

    if constexpr (std::is_same_v<T, uint8>) {
      FIntPoint vec2 = UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
          value,
          FIntPoint(0));
      for (int64 j = 0; j < 2; ++j) {
        *(pWritePos + j) =
            CesiumGltf::MetadataConversions<uint8, int32>::convert(vec2[j])
                .value_or(0);
      }
    } else if constexpr (std::is_same_v<T, float>) {
      FVector2D vec2 = UCesiumMetadataValueBlueprintLibrary::GetVector2D(
          value,
          FVector2D::Zero());

      // Floats are encoded backwards (e.g., ABGR)
      float* pWritePosF = reinterpret_cast<float*>(pWritePos + pixelSize) - 1;
      for (int64 j = 0; j < 2; ++j) {
        *pWritePosF =
            CesiumGltf::MetadataConversions<float, double>::convert(vec2[j])
                .value_or(0.0f);
        --pWritePosF;
      }
    }

    pWritePos += pixelSize;
  }
}

template <typename T>
void coerceAndEncodeVec3s(
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData,
    size_t pixelSize) {
  int64 propertySize =
      UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(property);
  if (textureData.size() < propertySize * 3 * sizeof(T)) {
    throw std::runtime_error(
        "Buffer is too small to store the data of this property.");
  }

  uint8* pWritePos = reinterpret_cast<uint8*>(textureData.data());

  for (int64 i = 0; i < propertySize; ++i) {
    FCesiumMetadataValue value =
        UCesiumPropertyTablePropertyBlueprintLibrary::GetRawValue(property, i);

    if constexpr (std::is_same_v<T, uint8>) {
      FIntVector vec3 = UCesiumMetadataValueBlueprintLibrary::GetIntVector(
          value,
          FIntVector(0));
      for (int64 j = 0; j < 3; ++j) {
        *(pWritePos + j) =
            CesiumGltf::MetadataConversions<uint8, int32>::convert(vec3[j])
                .value_or(0);
      }
    } else if constexpr (std::is_same_v<T, float>) {
      FVector3f vec3 = UCesiumMetadataValueBlueprintLibrary::GetVector3f(
          value,
          FVector3f::Zero());

      // Floats are encoded backwards (e.g., ABGR)
      float* pWritePosF = reinterpret_cast<float*>(pWritePos + pixelSize) - 1;
      for (int64 j = 0; j < 3; ++j) {
        *pWritePosF = vec3[j];
        --pWritePosF;
      }
    }

    pWritePos += pixelSize;
  }
}

template <typename T>
void coerceAndEncodeVec4s(
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData,
    size_t pixelSize) {
  int64 propertySize =
      UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(property);
  if (textureData.size() < propertySize * 4 * sizeof(T)) {
    throw std::runtime_error(
        "Buffer is too small to store the data of this property.");
  }

  uint8* pWritePos = reinterpret_cast<uint8*>(textureData.data());

  for (int64 i = 0; i < propertySize; ++i) {
    FCesiumMetadataValue value =
        UCesiumPropertyTablePropertyBlueprintLibrary::GetRawValue(property, i);
    FVector4 vec4 = UCesiumMetadataValueBlueprintLibrary::GetVector4(
        value,
        FVector4::Zero());

    if constexpr (std::is_same_v<T, uint8>) {
      for (int64 j = 0; j < 4; ++j) {
        *(pWritePos + j) =
            CesiumGltf::MetadataConversions<uint8, double>::convert(vec4[j])
                .value_or(0);
      }
    } else if constexpr (std::is_same_v<T, float>) {
      // Floats are encoded backwards (e.g., ABGR)
      float* pWritePosF = reinterpret_cast<float*>(pWritePos + pixelSize) - 1;
      for (int64 j = 0; j < 4; ++j) {
        *pWritePosF =
            CesiumGltf::MetadataConversions<float, double>::convert(vec4[j])
                .value_or(0.0f);
        --pWritePosF;
      }
    }

    pWritePos += pixelSize;
  }
}
} // namespace

bool CesiumEncodedMetadataCoerce::canEncode(
    const FCesiumPropertyTablePropertyDescription& description) {
  const ECesiumMetadataType type = description.PropertyDetails.Type;

  if (type == ECesiumMetadataType::Boolean ||
      type == ECesiumMetadataType::String) {
    // Booleans and boolean arrays are supported.
    // Strings and string arrays are technically supported for all encoded
    // types. This will attempt to coerce a string by parsing it as the
    // specified encoded type. If coercion fails, they default to zero values.
    return true;
  }

  const ECesiumMetadataComponentType componentType =
      description.PropertyDetails.ComponentType;
  if (componentType == ECesiumMetadataComponentType::None) {
    // Can't coerce a numeric property that doesn't know its component type.
    return false;
  }

  if (description.PropertyDetails.bIsArray) {
    // Only scalar and boolean types are supported. (Booleans will have been
    // verified earlier in this function).
    return type == ECesiumMetadataType::Scalar;
  }

  switch (type) {
  case ECesiumMetadataType::Scalar:
    // Scalars can be converted to vecNs.
    return true;
  case ECesiumMetadataType::Vec2:
  case ECesiumMetadataType::Vec3:
  case ECesiumMetadataType::Vec4:
    // VecNs can be converted to other vecNs of different dimensions, but not to
    // scalars.
    return description.EncodingDetails.Type !=
           ECesiumEncodedMetadataType::Scalar;
  default:
    return false;
  }
}

void CesiumEncodedMetadataCoerce::encode(
    const FCesiumPropertyTablePropertyDescription& propertyDescription,
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData,
    size_t pixelSize) {
  if (propertyDescription.PropertyDetails.bIsArray) {
    if (propertyDescription.EncodingDetails.ComponentType ==
        ECesiumEncodedMetadataComponentType::Uint8) {
      coerceAndEncodeArrays<uint8>(
          propertyDescription,
          property,
          textureData,
          pixelSize);
    } else if (
        propertyDescription.EncodingDetails.ComponentType ==
        ECesiumEncodedMetadataComponentType::Float) {
      coerceAndEncodeArrays<float>(
          propertyDescription,
          property,
          textureData,
          pixelSize);
    }
    return;
  }

  if (propertyDescription.EncodingDetails.ComponentType ==
      ECesiumEncodedMetadataComponentType::Uint8) {
    switch (propertyDescription.EncodingDetails.Type) {
    case ECesiumEncodedMetadataType::Scalar:
      coerceAndEncodeScalars<uint8>(property, textureData);
      break;
    case ECesiumEncodedMetadataType::Vec2:
      coerceAndEncodeVec2s<uint8>(property, textureData, pixelSize);
      break;
    case ECesiumEncodedMetadataType::Vec3:
      coerceAndEncodeVec3s<uint8>(property, textureData, pixelSize);
      break;
    case ECesiumEncodedMetadataType::Vec4:
      coerceAndEncodeVec4s<uint8>(property, textureData, pixelSize);
      break;
    default:
      break;
    }
  } else if (
      propertyDescription.EncodingDetails.ComponentType ==
      ECesiumEncodedMetadataComponentType::Float) {
    switch (propertyDescription.EncodingDetails.Type) {
    case ECesiumEncodedMetadataType::Scalar:
      coerceAndEncodeScalars<float>(property, textureData);
      break;
    case ECesiumEncodedMetadataType::Vec2:
      coerceAndEncodeVec2s<float>(property, textureData, pixelSize);
      break;
    case ECesiumEncodedMetadataType::Vec3:
      coerceAndEncodeVec3s<float>(property, textureData, pixelSize);
      break;
    case ECesiumEncodedMetadataType::Vec4:
      coerceAndEncodeVec4s<float>(property, textureData, pixelSize);
      break;
    default:
      break;
    }
  }
}

namespace {
/**
 * @param hexString The string containing the hex code color, including the #
 * prefix.
 */
glm::u8vec3 getHexColorFromString(const FString& hexString) {
  glm::u8vec3 result(0);

  // Get the code without the # sign
  FString hexSubstr = hexString.Mid(1);
  std::string hexStr = TCHAR_TO_UTF8(*hexSubstr);
  size_t length = hexStr.length();
  if (length != 3 && length != 6) {
    return result;
  }

  size_t substringLength = length / 3;
  for (int32 i = 0; i < 3; i++) {
    std::string substr = hexStr.substr(i * substringLength, substringLength);
    int32_t component = std::stoi(substr, 0, 16);
    result[i] =
        CesiumGltf::MetadataConversions<uint8, int32_t>::convert(component)
            .value_or(0);
  }

  return result;
}

/**
 * @param rgbString The string containing the rgb color in its original rgb(R,
 * G, B) format.
 */
glm::u8vec3 getRgbColorFromString(const FString& rgbString) {
  glm::u8vec3 result(0);

  TArray<FString> parts;
  parts.Reserve(3);

  int partCount = rgbString.Mid(4, rgbString.Len() - 5)
                      .ParseIntoArray(parts, TEXT(","), false);
  if (partCount == 3) {
    result[0] = FCString::Atoi(*parts[0]);
    result[1] = FCString::Atoi(*parts[1]);
    result[2] = FCString::Atoi(*parts[2]);
  }

  return result;
}

template <typename T>
void parseAndEncodeColors(
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData,
    size_t pixelSize) {
  int64 propertySize =
      UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(property);
  if (textureData.size() < propertySize * 3 * sizeof(T)) {
    throw std::runtime_error(
        "Buffer is too small to store the data of this property.");
  }

  uint8* pWritePos = reinterpret_cast<uint8*>(textureData.data());

  for (int64 i = 0; i < propertySize; i++) {
    FString str =
        UCesiumPropertyTablePropertyBlueprintLibrary::GetString(property, i);

    // This could be expanded to handle float or vec4 color representations.
    glm::u8vec3 color(0);

    if (str.StartsWith(TEXT("#"))) {
      // Handle hex case
      color = getHexColorFromString(str);
    } else if (str.StartsWith(TEXT("rgb(")) && str.EndsWith(TEXT(")"))) {
      // Handle rgb(R,G,B) case
      color = getRgbColorFromString(str);
    }

    if constexpr (std::is_same_v<T, uint8>) {
      for (int64 j = 0; j < 3; j++) {
        *(pWritePos + j) = color[j];
      }
    } else if constexpr (std::is_same_v<T, float>) {
      // Floats are encoded backwards (e.g., ABGR)
      float* pWritePosF = reinterpret_cast<float*>(pWritePos + pixelSize) - 1;
      for (int64 j = 0; j < 3; j++) {
        *pWritePosF =
            CesiumGltf::MetadataConversions<float, uint8_t>::convert(color[j])
                .value_or(0.0f);
        --pWritePosF;
      }
    }

    pWritePos += pixelSize;
  }
}
} // namespace

bool CesiumEncodedMetadataParseColorFromString::canEncode(
    const FCesiumPropertyTablePropertyDescription& description) {
  return description.PropertyDetails.Type == ECesiumMetadataType::String &&
         !description.PropertyDetails.bIsArray &&
         (description.EncodingDetails.Type ==
              ECesiumEncodedMetadataType::Vec3 ||
          description.EncodingDetails.Type == ECesiumEncodedMetadataType::Vec4);
}

void CesiumEncodedMetadataParseColorFromString::encode(
    const FCesiumPropertyTablePropertyDescription& propertyDescription,
    const FCesiumPropertyTableProperty& property,
    const std::span<std::byte>& textureData,
    size_t pixelSize) {
  if (propertyDescription.EncodingDetails.ComponentType ==
      ECesiumEncodedMetadataComponentType::Uint8) {
    parseAndEncodeColors<uint8>(property, textureData, pixelSize);
  } else if (
      propertyDescription.EncodingDetails.ComponentType ==
      ECesiumEncodedMetadataComponentType::Float) {
    parseAndEncodeColors<float>(property, textureData, pixelSize);
  }
}
