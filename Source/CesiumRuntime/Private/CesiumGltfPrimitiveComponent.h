// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "CesiumPrimitive.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
// #include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "VecMath.h"

#include "CesiumGltfPrimitiveComponent.generated.h"

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
} // namespace CesiumGltf

void UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform,
    UStaticMeshComponent* uobject);

UCLASS()
  class UCesiumGltfPrimitiveComponent :
   public UStaticMeshComponent, public ICesiumPrimitive {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfPrimitiveComponent();
  virtual ~UCesiumGltfPrimitiveComponent();

  void BeginDestroy() override;

  FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

  void resetPhysicsTransform() override;
};

UCLASS()
class UCesiumGltfInstancedComponent
  : public UInstancedStaticMeshComponent, public ICesiumPrimitive {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfInstancedComponent();
  virtual ~UCesiumGltfInstancedComponent();

  void BeginDestroy() override;

  FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

  void resetPhysicsTransform() override;
};
