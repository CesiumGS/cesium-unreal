#pragma once

#include <vector>
#include <array>
#include <cstdint>

/**
 * Standalone CGAL mesh operations.
 *
 * This interface uses plain C++ types so that callers (e.g. CesiumRuntime)
 * do not need to include any CGAL headers or compile with RTTI.
 */
namespace CesiumCgal {

/**
 * Clip a triangle mesh by a second (closed) mesh, keeping the interior.
 *
 * @param positions      Vertex positions of the input mesh (x,y,z triples).
 * @param indices        Triangle indices into @p positions.
 * @param clipPositions  Vertex positions of the clipping mesh.
 * @param clipIndices    Triangle indices into @p clipPositions.
 * @param outPositions   Receives the clipped vertex positions.
 * @param outIndices     Receives the clipped triangle indices.
 * @return true on success, false if the operation failed.
 */
bool ClipMesh(
    const std::vector<std::array<float, 3>>& positions,
    const std::vector<uint32_t>& indices,
    const std::vector<std::array<float, 3>>& clipPositions,
    const std::vector<uint32_t>& clipIndices,
    std::vector<std::array<float, 3>>& outPositions,
    std::vector<uint32_t>& outIndices);

enum class BooleanOp : uint8_t {
  Union,
  Intersection,
  Difference
};

/**
 * Perform a boolean operation between two closed triangle meshes.
 * Uses CGAL's corefine_and_compute_boolean_operations internally.
 *
 * @param positionsA     Vertex positions of mesh A.
 * @param indicesA       Triangle indices of mesh A.
 * @param positionsB     Vertex positions of mesh B.
 * @param indicesB       Triangle indices of mesh B.
 * @param op             The boolean operation to perform.
 * @param outPositions   Receives the result vertex positions.
 * @param outIndices     Receives the result triangle indices.
 * @return true on success, false if the operation failed.
 */
bool BooleanOperation(
    const std::vector<std::array<float, 3>>& positionsA,
    const std::vector<uint32_t>& indicesA,
    const std::vector<std::array<float, 3>>& positionsB,
    const std::vector<uint32_t>& indicesB,
    BooleanOp op,
    std::vector<std::array<float, 3>>& outPositions,
    std::vector<uint32_t>& outIndices);

} // namespace CesiumCgal
