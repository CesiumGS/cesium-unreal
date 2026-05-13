// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPrimitive.h"
#include "CesiumPrimitiveFeatures.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"

#include "CesiumGltfInstancedComponent.generated.h"

/**
 * A component that represents and renders an instanced glTF mesh primitive made
 * from triangles.
 */
UCLASS()
class UCesiumGltfInstancedComponent : public UInstancedStaticMeshComponent,
                                      public ICesiumPrimitive {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfInstancedComponent();
  virtual ~UCesiumGltfInstancedComponent();

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

  TSharedPtr<FCesiumPrimitiveFeatures> pInstanceFeatures;

private:
  CesiumPrimitiveData _cesiumData;
};
