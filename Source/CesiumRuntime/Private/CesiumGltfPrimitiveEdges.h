// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Templates/UniquePtr.h"

#include <CesiumGltf/AccessorView.h>

namespace CesiumGltf {
struct Model;
struct ExtensionExtMeshPrimitiveEdgeVisibility;
} // namespace CesiumGltf

class CesiumGltfPrimitiveEdges {
public:
  /**
   * Creates the render data necessary to visualize edges from the
   * ExtensionExtMeshPrimitiveEdgeVisibilty extension.
   */
  static TUniquePtr<FStaticMeshRenderData> createInWorkerThread(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& primitive,
      const CesiumGltf::AccessorView<FVector3f>& positionView,
      const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension);
};
