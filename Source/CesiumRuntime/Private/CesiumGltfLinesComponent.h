// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfLinesComponent.generated.h"

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

  /**
   * @brief The width of the lines specified by @ref
   * CesiumGltf::ExtensionMaterialBentleyMaterialsLineStyle, if present.
   */
  int64 width;

  /**
   * @brief The on/off bit pattern of the lines specified by @ref
   * CesiumGltf::ExtensionMaterialBentleyMaterialsLineStyle, if present.
   */
  uint16 pattern;

  // Override UPrimitiveComponent interface.
  virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
  virtual void OnCreatePhysicsState() override;
};
