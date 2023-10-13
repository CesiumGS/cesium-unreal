// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArray.h"

FCesiumPropertyArray::FCesiumPropertyArray() noexcept
    : _value(), _elementType() {}

FCesiumPropertyArray::FCesiumPropertyArray(
    const FCesiumPropertyArray& rhs) noexcept = default;

FCesiumPropertyArray::FCesiumPropertyArray(
    FCesiumPropertyArray&& rhs) noexcept = default;

FCesiumPropertyArray& FCesiumPropertyArray::operator=(
    const FCesiumPropertyArray& rhs) noexcept = default;

FCesiumPropertyArray&
FCesiumPropertyArray::operator=(FCesiumPropertyArray&& rhs) noexcept = default;

FCesiumPropertyArray::~FCesiumPropertyArray() noexcept = default;
