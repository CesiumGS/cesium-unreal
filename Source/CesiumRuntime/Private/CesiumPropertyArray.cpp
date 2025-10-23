// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArray.h"
#include "CesiumMetadataValue.h"

#include <CesiumGltf/PropertyTypeTraits.h>

FCesiumPropertyArray::FCesiumPropertyArray(FCesiumPropertyArray&& rhs) =
    default;

FCesiumPropertyArray&
FCesiumPropertyArray::operator=(FCesiumPropertyArray&& rhs) = default;

FCesiumPropertyArray::FCesiumPropertyArray(
    const TArray<FCesiumMetadataValue>&& values,
    TSharedPtr<FCesiumMetadataEnum> pEnumDefinition)
    : _valueView(),
      _valueArray(),
      _elementType(),
      _pEnumDefinition(pEnumDefinition) {
  if (values.Num() == 0) {
    return;
  }

  FCesiumMetadataValueType valueType =
      UCesiumMetadataValueBlueprintLibrary::GetValueType(values[0]);

  // Verify that all other elements in the array have the same type. If any of
  // them differ, consider this array invalid.
  for (int32 i = 1; i < values.Num(); i++) {
    FCesiumMetadataValueType otherValueType =
        UCesiumMetadataValueBlueprintLibrary::GetValueType(values[i]);
    if (valueType != otherValueType) {
      return;
    }
  }

  _elementType = valueType;
  _valueArray = std::move(values);
}

FCesiumPropertyArray::FCesiumPropertyArray(const FCesiumPropertyArray& rhs)
    : _valueView(),
      _valueArray(),
      _elementType(rhs._elementType),
      _storage(rhs._storage) {
  swl::visit(
      [this](const auto& value) {
        if constexpr (CesiumGltf::IsMetadataArray<decltype(value)>::value) {
          if (!this->_storage.empty()) {
            this->_valueView = decltype(value)(this->_storage);
          } else {
            this->_valueView = value;
          }
        } else {
          this->_valueView = value;
        }
      },
      rhs._valueView);
}

FCesiumPropertyArray&
FCesiumPropertyArray::operator=(const FCesiumPropertyArray& rhs) {
  *this = FCesiumPropertyArray(rhs);
  return *this;
}
