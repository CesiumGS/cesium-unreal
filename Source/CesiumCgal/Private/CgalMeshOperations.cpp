// This file is compiled with RTTI enabled, no PCH, and no unity build,
// so CGAL/Boost headers work without UE macro conflicts.

// Suppress warnings from CGAL/Boost headers that UBT treats as errors.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wshadow"

#include "CgalMeshOperations.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/clip.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point3 = Kernel::Point_3;
using Mesh = CGAL::Surface_mesh<Point3>;

namespace {

Mesh BuildSurfaceMesh(
    const std::vector<std::array<float, 3>>& positions,
    const std::vector<uint32_t>& indices) {
  Mesh mesh;
  for (const auto& p : positions) {
    mesh.add_vertex(Point3(p[0], p[1], p[2]));
  }
  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    mesh.add_face(
        Mesh::Vertex_index(indices[i]),
        Mesh::Vertex_index(indices[i + 1]),
        Mesh::Vertex_index(indices[i + 2]));
  }
  return mesh;
}

void ExtractMeshData(
    const Mesh& mesh,
    std::vector<std::array<float, 3>>& outPositions,
    std::vector<uint32_t>& outIndices) {
  outPositions.clear();
  outIndices.clear();

  // Build a mapping from vertex descriptors to contiguous indices,
  // since CGAL may have removed some vertices.
  std::unordered_map<Mesh::Vertex_index, uint32_t> vertexMap;
  for (auto v : mesh.vertices()) {
    uint32_t idx = static_cast<uint32_t>(outPositions.size());
    vertexMap[v] = idx;
    const Point3& pt = mesh.point(v);
    outPositions.push_back({
        static_cast<float>(pt.x()),
        static_cast<float>(pt.y()),
        static_cast<float>(pt.z())});
  }

  for (auto f : mesh.faces()) {
    auto h = mesh.halfedge(f);
    auto h0 = h;
    // Triangulate face (should already be triangles from clipping).
    std::vector<uint32_t> faceVerts;
    do {
      faceVerts.push_back(vertexMap[mesh.target(h)]);
      h = mesh.next(h);
    } while (h != h0);

    for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
      outIndices.push_back(faceVerts[0]);
      outIndices.push_back(faceVerts[i]);
      outIndices.push_back(faceVerts[i + 1]);
    }
  }
}

} // anonymous namespace

bool CesiumCgal::ClipMesh(
    const std::vector<std::array<float, 3>>& positions,
    const std::vector<uint32_t>& indices,
    const std::vector<std::array<float, 3>>& clipPositions,
    const std::vector<uint32_t>& clipIndices,
    std::vector<std::array<float, 3>>& outPositions,
    std::vector<uint32_t>& outIndices) {

  Mesh inputMesh = BuildSurfaceMesh(positions, indices);
  Mesh clipMesh = BuildSurfaceMesh(clipPositions, clipIndices);

  bool ok = CGAL::Polygon_mesh_processing::clip(
      inputMesh,
      clipMesh,
      CGAL::parameters::clip_volume(true));

  if (!ok) {
    return false;
  }

  ExtractMeshData(inputMesh, outPositions, outIndices);
  return true;
}

bool CesiumCgal::BooleanOperation(
    const std::vector<std::array<float, 3>>& positionsA,
    const std::vector<uint32_t>& indicesA,
    const std::vector<std::array<float, 3>>& positionsB,
    const std::vector<uint32_t>& indicesB,
    BooleanOp op,
    std::vector<std::array<float, 3>>& outPositions,
    std::vector<uint32_t>& outIndices) {

  Mesh meshA = BuildSurfaceMesh(positionsA, indicesA);
  Mesh meshB = BuildSurfaceMesh(positionsB, indicesB);
  Mesh result;

  bool ok = false;
  switch (op) {
  case BooleanOp::Union:
    ok = CGAL::Polygon_mesh_processing::corefine_and_compute_union(
        meshA, meshB, result);
    break;
  case BooleanOp::Intersection:
    ok = CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(
        meshA, meshB, result);
    break;
  case BooleanOp::Difference:
    ok = CGAL::Polygon_mesh_processing::corefine_and_compute_difference(
        meshA, meshB, result);
    break;
  }

  if (!ok) {
    return false;
  }

  ExtractMeshData(result, outPositions, outIndices);
  return true;
}

#pragma clang diagnostic pop
