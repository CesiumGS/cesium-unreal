// Copyright 2020-2021 CesiumGS, Inc. and Contributors

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

  // Override UPrimitiveComponent interface.
  virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
};
