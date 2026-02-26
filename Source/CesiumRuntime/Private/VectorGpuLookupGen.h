#pragma once

#include "Misc/Optional.h"

#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumVectorData/GeoJsonObject.h>

#include <glm/vec2.hpp>
#include <glm/ext/vector_uint2.hpp>

#include <vector>
#include <optional>

struct FCesiumVectorLookup
{
  std::vector<float> coords;
  std::vector<uint32_t> indices;
  std::vector<uint8_t> cutFlags;

  glm::uvec2 textureSize;
  glm::uvec2 gridSize;

  static std::optional<FCesiumVectorLookup> Create(
      CesiumVectorData::GeoJsonObject& rootObject,
      const CesiumGeospatial::GlobeRectangle& bbox,
      size_t targetLinePerCell = 1000,
      bool useTest = false,
      bool inGridClip = false);
};
