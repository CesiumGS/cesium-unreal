// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltfPrimitiveComponent.h"
#include "LoadGltfResult.h"

#include "CesiumGltfLinesComponent.generated.h"

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
struct ExtensionExtMeshPrimitiveEdgeVisibility;
} // namespace CesiumGltf

/**
 * A component that represents and renders a glTF lines primitive.
 */
UCLASS()
class UCesiumGltfLinesComponent : public UCesiumGltfPrimitiveComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfLinesComponent();
  virtual ~UCesiumGltfLinesComponent();

  // Override UPrimitiveComponent interface.
  virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
};

/**
 * Stores the data needed to render edges from the
 * `EXT_mesh_primitive_edge_visibility` extension.
 */
struct FCesiumMeshPrimitiveEdgeData {
  /**
   * Creates an `FCesiumMeshPrimitiveEdgeData` struct with data from the given
   * mesh primitive.
   */
  FCesiumMeshPrimitiveEdgeData(
      const CesiumGltf::Model& model,
      const CesiumGltf::MeshPrimitive& meshPrimitive,
      const CesiumGltf::ExtensionExtMeshPrimitiveEdgeVisibility& extension,
      LoadGltfResult::LoadedPrimitiveResult& primitiveResult);
  FCesiumMeshPrimitiveEdgeData(const FCesiumMeshPrimitiveEdgeData&) = delete;

  TUniquePtr<FStaticMeshRenderData> pEdgeRenderData = nullptr;
};
