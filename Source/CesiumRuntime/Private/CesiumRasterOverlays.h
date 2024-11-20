// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

constexpr size_t maximumOverlayTextureCoordinateIDs = 2;

/**
 * @brief Maps an overlay texture coordinate ID to the index of the
 * corresponding texture coordinates in the static mesh's UVs array.
 */
using OverlayTextureCoordinateIDMap = std::map<size_t, int32_t>;
