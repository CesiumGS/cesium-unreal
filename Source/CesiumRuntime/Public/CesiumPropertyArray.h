// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataValueType.h"
#include "UObject/ObjectMacros.h"
#include <swl/variant.hpp>
#include "CesiumPropertyArray.generated.h"

/**
 * A Blueprint-accessible wrapper for an array property in glTF metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyArray {
  GENERATED_USTRUCT_BODY()

private:
#pragma region ArrayType
  template <typename T>
  using ArrayPropertyView = CesiumGltf::PropertyArrayCopy<T>;

  using ArrayType = swl::variant<
      ArrayPropertyView<int8_t>,
      ArrayPropertyView<uint8_t>,
      ArrayPropertyView<int16_t>,
      ArrayPropertyView<uint16_t>,
      ArrayPropertyView<int32_t>,
      ArrayPropertyView<uint32_t>,
      ArrayPropertyView<int64_t>,
      ArrayPropertyView<uint64_t>,
      ArrayPropertyView<float>,
      ArrayPropertyView<double>,
      CesiumGltf::PropertyArrayView<bool>,
      CesiumGltf::PropertyArrayView<std::string_view>,
      ArrayPropertyView<glm::vec<2, int8_t>>,
      ArrayPropertyView<glm::vec<2, uint8_t>>,
      ArrayPropertyView<glm::vec<2, int16_t>>,
      ArrayPropertyView<glm::vec<2, uint16_t>>,
      ArrayPropertyView<glm::vec<2, int32_t>>,
      ArrayPropertyView<glm::vec<2, uint32_t>>,
      ArrayPropertyView<glm::vec<2, int64_t>>,
      ArrayPropertyView<glm::vec<2, uint64_t>>,
      ArrayPropertyView<glm::vec<2, float>>,
      ArrayPropertyView<glm::vec<2, double>>,
      ArrayPropertyView<glm::vec<3, int8_t>>,
      ArrayPropertyView<glm::vec<3, uint8_t>>,
      ArrayPropertyView<glm::vec<3, int16_t>>,
      ArrayPropertyView<glm::vec<3, uint16_t>>,
      ArrayPropertyView<glm::vec<3, int32_t>>,
      ArrayPropertyView<glm::vec<3, uint32_t>>,
      ArrayPropertyView<glm::vec<3, int64_t>>,
      ArrayPropertyView<glm::vec<3, uint64_t>>,
      ArrayPropertyView<glm::vec<3, float>>,
      ArrayPropertyView<glm::vec<3, double>>,
      ArrayPropertyView<glm::vec<4, int8_t>>,
      ArrayPropertyView<glm::vec<4, uint8_t>>,
      ArrayPropertyView<glm::vec<4, int16_t>>,
      ArrayPropertyView<glm::vec<4, uint16_t>>,
      ArrayPropertyView<glm::vec<4, int32_t>>,
      ArrayPropertyView<glm::vec<4, uint32_t>>,
      ArrayPropertyView<glm::vec<4, int64_t>>,
      ArrayPropertyView<glm::vec<4, uint64_t>>,
      ArrayPropertyView<glm::vec<4, float>>,
      ArrayPropertyView<glm::vec<4, double>>,
      ArrayPropertyView<glm::mat<2, 2, int8_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint8_t>>,
      ArrayPropertyView<glm::mat<2, 2, int16_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint16_t>>,
      ArrayPropertyView<glm::mat<2, 2, int32_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint32_t>>,
      ArrayPropertyView<glm::mat<2, 2, int64_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint64_t>>,
      ArrayPropertyView<glm::mat<2, 2, float>>,
      ArrayPropertyView<glm::mat<2, 2, double>>,
      ArrayPropertyView<glm::mat<3, 3, int8_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint8_t>>,
      ArrayPropertyView<glm::mat<3, 3, int16_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint16_t>>,
      ArrayPropertyView<glm::mat<3, 3, int32_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint32_t>>,
      ArrayPropertyView<glm::mat<3, 3, int64_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint64_t>>,
      ArrayPropertyView<glm::mat<3, 3, float>>,
      ArrayPropertyView<glm::mat<3, 3, double>>,
      ArrayPropertyView<glm::mat<4, 4, int8_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint8_t>>,
      ArrayPropertyView<glm::mat<4, 4, int16_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint16_t>>,
      ArrayPropertyView<glm::mat<4, 4, int32_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint32_t>>,
      ArrayPropertyView<glm::mat<4, 4, int64_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint64_t>>,
      ArrayPropertyView<glm::mat<4, 4, float>>,
      ArrayPropertyView<glm::mat<4, 4, double>>>;
#pragma endregion

public:
  /**
   * Constructs an empty array instance with an unknown element type.
   */
  FCesiumPropertyArray() : _value(), _elementType() {}

  /**
   * Constructs an array instance.
   * @param value The property array view that will be stored in this struct
   */
  template <typename T>
  FCesiumPropertyArray(CesiumGltf::PropertyArrayCopy<T> value)
      : _value(value), _elementType() {
    ECesiumMetadataType type =
        ECesiumMetadataType(CesiumGltf::TypeToPropertyType<T>::value);
    ECesiumMetadataComponentType componentType = ECesiumMetadataComponentType(
        CesiumGltf::TypeToPropertyType<T>::component);
    bool isArray = false;

    _elementType = {type, componentType, isArray};
  }

  template <typename T>
  FCesiumPropertyArray(CesiumGltf::PropertyArrayView<T> value)
      : _value(value), _elementType() {
    ECesiumMetadataType type =
        ECesiumMetadataType(CesiumGltf::TypeToPropertyType<T>::value);
    ECesiumMetadataComponentType componentType = ECesiumMetadataComponentType(
        CesiumGltf::TypeToPropertyType<T>::component);
    bool isArray = false;

    _elementType = {type, componentType, isArray};
  }

private:
  template <typename T, typename... VariantType>
  static bool
  holdsArrayAlternative(const swl::variant<VariantType...>& variant) {
    return swl::holds_alternative<CesiumGltf::PropertyArrayView<T>>(variant);
  }

  ArrayType _value;
  FCesiumMetadataValueType _elementType;

  friend class UCesiumPropertyArrayBlueprintLibrary;
};
