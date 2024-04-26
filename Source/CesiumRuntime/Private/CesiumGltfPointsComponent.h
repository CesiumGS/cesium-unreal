// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfPointsComponent.generated.h"

UCLASS()
class UCesiumGltfPointsComponent : public UCesiumGltfPrimitiveComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfPointsComponent();
  virtual ~UCesiumGltfPointsComponent();

  // Whether the tile that contains this point component uses additive
  // refinement.
  bool UsesAdditiveRefinement;

  // The geometric error of the tile containing this point component.
  float GeometricError;

  // The dimensions of the point component. Used to estimate the geometric
  // error.
  glm::vec3 Dimensions;

  // Override UPrimitiveComponent interface.
  virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
};
