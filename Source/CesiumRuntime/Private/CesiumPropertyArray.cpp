// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArray.h"
#include <CesiumGltf/PropertyTypeTraits.h>

using namespace CesiumGltf;

FCesiumPropertyArray::FCesiumPropertyArray(FCesiumPropertyArray&& rhs) =
    default;

FCesiumPropertyArray&
FCesiumPropertyArray::operator=(FCesiumPropertyArray&& rhs) = default;

FCesiumPropertyArray::FCesiumPropertyArray(const FCesiumPropertyArray& rhs)
    : _value(), _elementType(rhs._elementType), _storage(rhs._storage) {
  swl::visit(
      [this](const auto& value) {
        if constexpr (IsMetadataArray<decltype(value)>::value) {
          if (!this->_storage.empty()) {
            this->_value = decltype(value)(this->_storage);
          } else {
            this->_value = value;
          }
        } else {
          this->_value = value;
        }
      },
      rhs._value);
}

FCesiumPropertyArray&
FCesiumPropertyArray::operator=(const FCesiumPropertyArray& rhs) {
  *this = FCesiumPropertyArray(rhs);
  return *this;
}
