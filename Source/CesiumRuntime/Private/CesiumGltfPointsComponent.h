// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfPointsComponent.generated.h"

/**
 * A component that represents and renders a glTF points primitive.
 */
UCLASS()
class UCesiumGltfPointsComponent : public UCesiumGltfPrimitiveComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfPointsComponent();
  virtual ~UCesiumGltfPointsComponent();

  /**
   * @brief Whether the tile that contains this point component uses additive
   * refinement. This affects how attenuation is computed for the component.
   */
  bool usesAdditiveRefinement;

  /**
   * @brief The geometric error of the tile containing this point component.
   * Affects how attenuation is computed for the component.
   */
  float geometricError;

  /**
   * @brief The dimensions of the point component. Used to estimate the
   * geometric error if none was specified.
   */
  glm::vec3 dimensions;

  /**
   * @brief The diameter specified by @ref
   * CesiumGltf::ExtensionBentleyMaterialsPointStyle, if present.
   */
  int64 diameter;

  // Override UPrimitiveComponent interface.
  virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
  virtual void OnCreatePhysicsState() override;
};
