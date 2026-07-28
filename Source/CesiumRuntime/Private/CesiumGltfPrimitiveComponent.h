// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPrimitive.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"

#include "CesiumGltfPrimitiveComponent.generated.h"

/**
 * A component that represents and renders a glTF mesh primitive made
 * from triangles.
 */
UCLASS()
class UCesiumGltfPrimitiveComponent : public UStaticMeshComponent,
                                      public ICesiumPrimitive {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfPrimitiveComponent();
  virtual ~UCesiumGltfPrimitiveComponent();

  void BeginDestroy() override;

  FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

  // from ICesiumPrimitive
  CesiumPrimitiveData& getPrimitiveData() override;
  const CesiumPrimitiveData& getPrimitiveData() const override;
  void
  UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform) override;

  // from ICesiumLoadedTilePrimitive
  ICesiumLoadedTile& GetLoadedTile() override;
  UStaticMeshComponent& GetMeshComponent() override;
  std::optional<uint32_t> FindTextureCoordinateIndexForGltfAccessor(
      int32_t accessorIndex) const override;

  virtual void OnCreatePhysicsState() override;

private:
  CesiumPrimitiveData _cesiumData;
};
