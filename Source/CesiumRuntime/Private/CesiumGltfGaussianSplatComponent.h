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

  // The dimensions of the point component. Used to estimate the geometric
  // error.
  glm::vec3 Dimensions;

  virtual void
  UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform) override;

  virtual void OnUpdateTransform(
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport) override;

  void
  SetData(CesiumGltf::Model& model, CesiumGltf::MeshPrimitive& meshPrimitive);

  void RegisterWithSubsystem();

  FBox GetBounds() const;

  glm::mat4x4 GetMatrix() const;

  virtual void BeginDestroy() override;

  TArray<float> Positions;
  TArray<float> Scales;
  TArray<float> Orientations;
  TArray<float> Colors;
  TArray<float> SphericalHarmonics;

  int32 NumCoefficients = 0;

  int32 NumSplats = 0;

private:
  TOptional<FBox> Bounds;
};
