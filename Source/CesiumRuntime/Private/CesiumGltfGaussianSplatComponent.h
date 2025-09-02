// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltfPrimitiveComponent.h"

#include <CesiumGltf/MeshPrimitive.h>

#include <glm/mat4x4.hpp>

#include "CesiumGltfGaussianSplatComponent.generated.h"

/**
 * A component that represents and renders a glTF gaussian splat.
 */
UCLASS()
class UCesiumGltfGaussianSplatComponent : public UCesiumGltfPrimitiveComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfGaussianSplatComponent();
  virtual ~UCesiumGltfGaussianSplatComponent();

  // Whether the tile that contains this point component uses additive
  // refinement.
  bool UsesAdditiveRefinement;

  // The geometric error of the tile containing this point component.
  float GeometricError;

  // The dimensions of the point component. Used to estimate the geometric
  // error.
  glm::vec3 Dimensions;

  void
  SetData(CesiumGltf::Model& model, CesiumGltf::MeshPrimitive& meshPrimitive);

  FBox GetBounds() const;

  glm::mat4x4 GetMatrix() const;

  virtual void BeginPlay() override;
  virtual void BeginDestroy() override;

private:
  /**
   *  Every piece of data required to render this splat.
   *
   * Layout:
   * - 0, 1, 2 - Position XYZ
   * - 3, 4, 5 - Scale XYZ
   * - 6, 7, 8, 9 - Orientation XYZW
   * - 10, 11, 12, 13 - Color RGBA
   * - 14, 15, 16 - SH deg 1 coeff 0
   * - (and so on for rest of SH coeffs)
   */
  TArray<float> Data;

  TOptional<FBox> Bounds;
};
