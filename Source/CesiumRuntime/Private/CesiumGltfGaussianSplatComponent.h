// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#pragma once

#include "CesiumGltfPrimitiveComponent.h"

#include "Math/Box.h"
#include "Misc/Optional.h"

#include <CesiumGltf/MeshPrimitive.h>

#include <glm/mat4x4.hpp>

#include "CesiumGltfGaussianSplatComponent.generated.h"

/**
 * Stores the data (positions, orientations, colors, etc) needed to render a
 * gaussian splat glTF.
 */
USTRUCT()
struct FCesiumGltfGaussianSplatData {
  GENERATED_BODY()

  /**
   * The position data of this gaussian splat. This will have `NumSplats * 4`
   * values, laid out in sequential XYZW order. The W component will always be
   * zero.
   */
  TArray<float> Positions;

  /**
   * The scale data of this gaussian splat. This will have `NumSplats * 4`
   * values, laid out in sequential XYZW order. The W component will always be
   * zero.
   */
  TArray<float> Scales;

  /**
   * The orientation data of this gaussian splat. This will have `NumSplats * 4`
   * values, laid out in sequential XYZW order.
   */
  TArray<float> Orientations;

  /**
   * The orientation data of this gaussian splat. This will have `NumSplats * 4`
   * values, laid out in sequential RGBA order.
   */
  TArray<float> Colors;

  /**
   * The orientation data of this gaussian splat. This will have `NumSplats *
   * NumCoefficients * 4` values, laid out in sequential XYZW order. For
   * example, for a splat with second degree spherical harmonics, the data will
   * be laid out like:
   * [ shd1_0, shd1_1, shd1_2, shd2_0, shd2_1, shd2_2, shd2_3, shd2_4 ]
   * where each `shdX_X` value is a set of four XYZW values. This order then
   * repeats for the next splat, and so on
   */
  TArray<float> SphericalHarmonics;

  /**
   * The bounds of this splat data in local space.
   */
  TOptional<FBox> Bounds;

  /**
   * The number of spherical harmonic coefficients contained in this data. This
   * will be either 0, 3, 8, or 15.
   */
  int32 NumCoefficients = 0;

  /**
   * The number of splats contained in this data.
   */
  int32 NumSplats = 0;

  /**
   * Creates an `FCesiumGltfGaussianSplatData` struct with no data.
   */
  FCesiumGltfGaussianSplatData() {}

  /**
   * Creates an `FCesiumGltfGaussianSplatData` struct with data from the given
   * mesh primitive.
   */
  FCesiumGltfGaussianSplatData(
      CesiumGltf::Model& model,
      CesiumGltf::MeshPrimitive& meshPrimitive);
};

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

  virtual void
  UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform) override;

  virtual void OnVisibilityChanged() override;

  virtual void OnUpdateTransform(
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport) override;

  /**
   * Registers this splat with the `UCesiumGaussianSplatSubsystem` so that it
   * will be considered for rendering.
   *
   * This will be called by `CesiumGltfComponent` when constructing a splat
   * glTF. You should not need to call it yourself.
   */
  void RegisterWithSubsystem();

  /**
   * Returns the bounds of the gaussian splat data in local space.
   */
  FBox GetBounds() const;

  /**
   * Returns the transformation matrix of this glTF component as a
   * `glm::mat4x4`.
   */
  glm::mat4x4 GetMatrix() const;

  virtual void BeginDestroy() override;

  /**
   * The gaussian splat data that will be used to render this component.
   */
  FCesiumGltfGaussianSplatData Data;
};
