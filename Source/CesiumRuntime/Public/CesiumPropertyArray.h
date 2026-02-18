// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataEnum.h"
#include "CesiumMetadataValueType.h"
#include "UObject/ObjectMacros.h"

#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <swl/variant.hpp>

#include "CesiumPropertyArray.generated.h"

struct FCesiumMetadataValue;

/**
 * A Blueprint-accessible wrapper for an array value from 3D Tiles or glTF
 * metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyArray {
  GENERATED_USTRUCT_BODY()

private:
#pragma region ArrayType
  template <typename T> using ArrayView = CesiumGltf::PropertyArrayView<T>;

  using ArrayType = swl::variant<
      ArrayView<int8_t>,
      ArrayView<uint8_t>,
      ArrayView<int16_t>,
      ArrayView<uint16_t>,
      ArrayView<int32_t>,
      ArrayView<uint32_t>,
      ArrayView<int64_t>,
      ArrayView<uint64_t>,
      ArrayView<float>,
      ArrayView<double>,
      ArrayView<bool>,
      ArrayView<std::string_view>,
      ArrayView<glm::vec<2, int8_t>>,
      ArrayView<glm::vec<2, uint8_t>>,
      ArrayView<glm::vec<2, int16_t>>,
      ArrayView<glm::vec<2, uint16_t>>,
      ArrayView<glm::vec<2, int32_t>>,
      ArrayView<glm::vec<2, uint32_t>>,
      ArrayView<glm::vec<2, int64_t>>,
      ArrayView<glm::vec<2, uint64_t>>,
      ArrayView<glm::vec<2, float>>,
      ArrayView<glm::vec<2, double>>,
      ArrayView<glm::vec<3, int8_t>>,
      ArrayView<glm::vec<3, uint8_t>>,
      ArrayView<glm::vec<3, int16_t>>,
      ArrayView<glm::vec<3, uint16_t>>,
      ArrayView<glm::vec<3, int32_t>>,
      ArrayView<glm::vec<3, uint32_t>>,
      ArrayView<glm::vec<3, int64_t>>,
      ArrayView<glm::vec<3, uint64_t>>,
      ArrayView<glm::vec<3, float>>,
      ArrayView<glm::vec<3, double>>,
      ArrayView<glm::vec<4, int8_t>>,
      ArrayView<glm::vec<4, uint8_t>>,
      ArrayView<glm::vec<4, int16_t>>,
      ArrayView<glm::vec<4, uint16_t>>,
      ArrayView<glm::vec<4, int32_t>>,
      ArrayView<glm::vec<4, uint32_t>>,
      ArrayView<glm::vec<4, int64_t>>,
      ArrayView<glm::vec<4, uint64_t>>,
      ArrayView<glm::vec<4, float>>,
      ArrayView<glm::vec<4, double>>,
      ArrayView<glm::mat<2, 2, int8_t>>,
      ArrayView<glm::mat<2, 2, uint8_t>>,
      ArrayView<glm::mat<2, 2, int16_t>>,
      ArrayView<glm::mat<2, 2, uint16_t>>,
      ArrayView<glm::mat<2, 2, int32_t>>,
      ArrayView<glm::mat<2, 2, uint32_t>>,
      ArrayView<glm::mat<2, 2, int64_t>>,
      ArrayView<glm::mat<2, 2, uint64_t>>,
      ArrayView<glm::mat<2, 2, float>>,
      ArrayView<glm::mat<2, 2, double>>,
      ArrayView<glm::mat<3, 3, int8_t>>,
      ArrayView<glm::mat<3, 3, uint8_t>>,
      ArrayView<glm::mat<3, 3, int16_t>>,
      ArrayView<glm::mat<3, 3, uint16_t>>,
      ArrayView<glm::mat<3, 3, int32_t>>,
      ArrayView<glm::mat<3, 3, uint32_t>>,
      ArrayView<glm::mat<3, 3, int64_t>>,
      ArrayView<glm::mat<3, 3, uint64_t>>,
      ArrayView<glm::mat<3, 3, float>>,
      ArrayView<glm::mat<3, 3, double>>,
      ArrayView<glm::mat<4, 4, int8_t>>,
      ArrayView<glm::mat<4, 4, uint8_t>>,
      ArrayView<glm::mat<4, 4, int16_t>>,
      ArrayView<glm::mat<4, 4, uint16_t>>,
      ArrayView<glm::mat<4, 4, int32_t>>,
      ArrayView<glm::mat<4, 4, uint32_t>>,
      ArrayView<glm::mat<4, 4, int64_t>>,
      ArrayView<glm::mat<4, 4, uint64_t>>,
      ArrayView<glm::mat<4, 4, float>>,
      ArrayView<glm::mat<4, 4, double>>>;
#pragma endregion

public:
  /**
   * Constructs an empty instance with an unknown element type.
   */
  FCesiumPropertyArray();

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
        _elementType(TypeToMetadataValueType<T>(pEnumDefinition)),
        _storage(),
        _pEnumDefinition(pEnumDefinition) {
    this->_value = std::move(value).toViewAndExternalBuffer(this->_storage);
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
      : _value(value),
        _elementType(TypeToMetadataValueType<T>(pEnumDefinition)),
        _pEnumDefinition(pEnumDefinition) {}

  FCesiumPropertyArray(FCesiumPropertyArray&& rhs);
  FCesiumPropertyArray& operator=(FCesiumPropertyArray&& rhs);
  FCesiumPropertyArray(const FCesiumPropertyArray& rhs);
  FCesiumPropertyArray& operator=(const FCesiumPropertyArray& rhs);

private:
  ArrayType _value;
  FCesiumMetadataValueType _elementType;
  std::vector<std::byte> _storage;
  TSharedPtr<FCesiumMetadataEnum> _pEnumDefinition;

  friend class UCesiumPropertyArrayBlueprintLibrary;
  friend struct FCesiumMetadataValue;
};
