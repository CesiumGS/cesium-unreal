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

  // Override UPrimitiveComponent interface.
  virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
};
