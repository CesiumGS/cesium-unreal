#pragma once

#include "CesiumGltf/AccessorView.h"
#include <variant>

/**
 * Type definition for a position accessor.
 */
typedef CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC3<float>>
    CesiumPositionAccessorType;

/**
 * Visitor that retrieves the count of elements in the given accessor type as an
 * int64.
 */
struct CesiumCountFromAccessor {
  int64 operator()(std::monostate) { return 0; }

  template <typename T>
  int64 operator()(const CesiumGltf::AccessorView<T>& value) {
    return static_cast<int64>(value.size());
  }
};

/**
 * Type definition for all kinds of feature ID accessors.
 */
typedef std::variant<
    CesiumGltf::AccessorView<int8_t>,
    CesiumGltf::AccessorView<uint8_t>,
    CesiumGltf::AccessorView<int16_t>,
    CesiumGltf::AccessorView<uint16_t>,
    CesiumGltf::AccessorView<uint32_t>,
    CesiumGltf::AccessorView<float>>
    CesiumFeatureIDAccessorType;

/**
 * Visitor that retrieves the feature ID from the given accessor type as an
 * int64. This should be initialized with the index of the vertex whose feature
 * ID is being queried.
 *
 * -1 is used to indicate errors retrieving the feature ID, e.g., if the given
 * index was out-of-bounds.
 */
struct CesiumFeatureIDFromAccessor {
  int64 operator()(const CesiumGltf::AccessorView<float>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }
    return static_cast<int64>(glm::round(value[index]));
  }

  template <typename T>
  int64 operator()(const CesiumGltf::AccessorView<T>& value) {
    if (index < 0 || index >= value.size()) {
      return -1;
    }
    return static_cast<int64>(value[index]);
  }

  int64 index;
};

/**
 * Type definition for all kinds of index accessors. std::monostate
 * indicates a nonexistent accessor, which can happen (and is valid) if the
 * primitive vertices are defined without an index buffer.
 */
typedef std::variant<
    std::monostate,
    CesiumGltf::AccessorView<uint8_t>,
    CesiumGltf::AccessorView<uint16_t>,
    CesiumGltf::AccessorView<uint32_t>>
    CesiumIndexAccessorType;

/**
 * Visitor that retrieves the vertex indices from the given accessor type
 * corresponding to a given face index. These indices are returned as an array
 * of int64s. This should be initialized with the index of face and the total
 * number of vertices in the primitive.
 *
 * -1 is used to indicate errors retrieving the index, e.g., if the given
 * index was out-of-bounds.
 */
struct CesiumFaceVertexIndicesFromAccessor {
  std::array<int64, 3> operator()(std::monostate) {
    const int64 firstVertex = faceIndex * 3;
    std::array<int64, 3> result;

    for (int64 i = 0; i < 3; i++) {
      int64 vertexIndex = firstVertex + i;
      result[i] = vertexIndex < vertexCount ? vertexIndex : -1;
    }

    return result;
  }

  template <typename T>
  std::array<int64, 3> operator()(const CesiumGltf::AccessorView<T>& value) {
    const int64 firstVertex = faceIndex * 3;
    std::array<int64, 3> result;

    for (int64 i = 0; i < 3; i++) {
      int64 vertexIndex = firstVertex + i;
      result[i] = vertexIndex < value.size()
                      ? static_cast<int64>(value[vertexIndex])
                      : -1;
    }

    return result;
  }

  int64 faceIndex;
  int64 vertexCount;
};

/**
 * Type definition for all kinds of texture coordinate (TEXCOORD_n) accessors.
 * std::monostate indicates a nonexistent or invalid accessor.
 */
typedef std::variant<
    std::monostate,
    CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint8_t>>,
    CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint16_t>>,
    CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>>
    CesiumTexCoordAccessorType;

/**
 * Retrieves an accessor view for the specified texture coordinate set from the
 * given glTF primitive and model. This verifies that the accessor is of a valid
 * type. If not, the returned accessor view will be invalid.,
 */
CesiumTexCoordAccessorType GetTexCoordAccessorView(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    int32 textureCoordinateSetIndex);

/**
 * Visitor that retrieves the texture coordinates from the given accessor type
 * as a glm::dvec2. This should be initialized with the target index.
 *
 * There are technically no invalid UV values because of clamp / wrap
 * behavior, so we use std::nullopt to denote an erroneous value.
 */
struct CesiumTexCoordFromAccessor {
  std::optional<glm::dvec2> operator()(std::monostate) { return std::nullopt; }

  std::optional<glm::dvec2> operator()(
      const CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>&
          value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    return glm::dvec2(value[index].value[0], value[index].value[1]);
  }

  template <typename T>
  std::optional<glm::dvec2>
  operator()(const CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<T>>&
                 value) {
    if (index < 0 || index >= value.size()) {
      return std::nullopt;
    }

    double u = static_cast<double>(value[index].value[0]);
    double v = static_cast<double>(value[index].value[1]);

    // TODO: do normalization logic in cesium-native?
    u /= std::numeric_limits<T>::max();
    v /= std::numeric_limits<T>::max();

    return glm::dvec2(u, v);
  }

  int64 index;
};
