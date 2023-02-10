// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumRasterOverlays.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <unordered_map>
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
