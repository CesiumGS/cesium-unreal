// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataEnum.h"
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
  using ArrayPropertyView = CesiumGltf::PropertyArrayView<T>;

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
      ArrayPropertyView<bool>,
      ArrayPropertyView<std::string_view>,
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
   * @param pEnumDefinition The enum definition this property uses, if any.
   */
  template <typename T>
  FCesiumPropertyArray(
      CesiumGltf::PropertyArrayCopy<T>&& value,
      TSharedPtr<FCesiumMetadataEnum> pEnumDefinition = nullptr)
      : _value(),
        _elementType(),
        _storage(),
        _pEnumDefinition(pEnumDefinition) {
    this->_value = std::move(value).toViewAndExternalBuffer(this->_storage);
    _elementType = TypeToMetadataValueType<T>(pEnumDefinition);
  }

  template <typename T>
  FCesiumPropertyArray(
      const CesiumGltf::PropertyArrayCopy<T>& value,
      TSharedPtr<FCesiumMetadataEnum> pEnumDefinition = nullptr)
      : FCesiumPropertyArray(
            CesiumGltf::PropertyArrayCopy<T>(value),
            pEnumDefinition) {}

  template <typename T>
  FCesiumPropertyArray(
      const CesiumGltf::PropertyArrayView<T>& value,
      TSharedPtr<FCesiumMetadataEnum> pEnumDefinition = nullptr)
      : _value(value), _elementType(), _pEnumDefinition(pEnumDefinition) {
    _elementType = TypeToMetadataValueType<T>(pEnumDefinition);
  }

  FCesiumPropertyArray(FCesiumPropertyArray&& rhs);
  FCesiumPropertyArray& operator=(FCesiumPropertyArray&& rhs);
  FCesiumPropertyArray(const FCesiumPropertyArray& rhs);
  FCesiumPropertyArray& operator=(const FCesiumPropertyArray& rhs);

private:
  template <typename T, typename... VariantType>
  static bool
  holdsArrayAlternative(const swl::variant<VariantType...>& variant) {
    return swl::holds_alternative<CesiumGltf::PropertyArrayView<T>>(variant);
  }

  ArrayType _value;
  FCesiumMetadataValueType _elementType;
  std::vector<std::byte> _storage;
  TSharedPtr<FCesiumMetadataEnum> _pEnumDefinition;

  friend class UCesiumPropertyArrayBlueprintLibrary;
};
