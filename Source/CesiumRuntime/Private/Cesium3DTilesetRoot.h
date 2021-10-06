// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "Cesium3DTilesetRoot.generated.h"

UCLASS()
class UCesium3DTilesetRoot : public USceneComponent {
  GENERATED_BODY()

public:
  UCesium3DTilesetRoot();

  /**
   * @brief Gets the transform from the "Cesium Tileset" reference frame to the
   * "Unreal Relative World" reference frame.
   *
   * Gets a matrix that transforms coordinates from the "Cesium Tileset"
   * reference frame (which is _usually_ Earth-centered, Earth-fixed) to Unreal
   * Engine's relative world coordinates (i.e. relative to the world
   * OriginLocation).
   *
   * See {@link reference-frames.md}.
   *
   * This transformation is a function of :
   *   * The location of the Tileset in "Unreal Absolute World" coordinates.
   *   * The rotation and scale of the tileset relative to the Unreal World.
   *   * `UWorld::OriginLocation`
   *   * The transformation from ellipsoid-centered to georeferenced
   * coordinates, as provided by `CesiumGeoreference`.
   */
  const glm::dmat4& GetCesiumTilesetToUnrealRelativeWorldTransform() const;

  virtual void
  ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

  UFUNCTION()
  void HandleGeoreferenceUpdated();

protected:
  virtual void BeginPlay() override;
  virtual bool MoveComponentImpl(
      const FVector& Delta,
      const FQuat& NewRotation,
      bool bSweep,
      FHitResult* OutHit = NULL,
      EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags,
      ETeleportType Teleport = ETeleportType::None) override;

private:
  void _updateAbsoluteLocation();
  void _updateTilesetToUnrealRelativeWorldTransform();

  glm::dvec3 _worldOriginLocation;
  glm::dvec3 _absoluteLocation;
  glm::dmat4 _tilesetToUnrealRelativeWorld;
};
