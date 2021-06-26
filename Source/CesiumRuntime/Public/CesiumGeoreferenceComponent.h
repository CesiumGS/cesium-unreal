// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreference.h"
#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>

#include "CesiumGeoreferenceComponent.generated.h"

/**
 * This component can be added to movable actors to globally georeference them
 * and maintain precise placement. When the owning actor is transformed through
 * normal Unreal Engine mechanisms, the internal geospatial coordinates will be
 * automatically updated. The actor position can also be set in terms of
 * Earth-Centered, Eath-Fixed coordinates (ECEF) or Longitude, Latitude, and
 * Height relative to the ellipsoid.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGeoreferenceComponent : public UActorComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGeoreferenceComponent();

  /**
   * The georeference actor controlling how the owning actor's coordinate system
   * relates to the coordinate system in this Unreal Engine level.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ACesiumGeoreference* Georeference;

  /**
   * Whether to automatically restore the precision of the Unreal transform from
   * the source Earth-Centered, Earth-Fixed (ECEF) transform during
   * origin-rebase. This is useful for maintaining high-precision for fixed
   * objects like buildings. This may need to be disabled for objects where the
   * Unreal transform is to be treated as the ground truth, e.g. Unreal physics
   * objects, cameras, etc.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool FixTransformOnOriginRebase = true;

  /**
   * Using the teleport flag will move objects to the updated transform
   * immediately and without affecting their velocity. This is useful when
   * working with physics actors that maintain an internal velocity which we do
   * not want to change when updating location.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool TeleportWhenUpdatingTransform = true;

  /**
   * The latitude in degrees of this component, in the range [-90, 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -90.0, ClampMax = 90.0))
  double Latitude = 0.0;

  /**
   * The longitude in degrees of this component, in the range [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -180.0, ClampMax = 180.0))
  double Longitude = 0.0;

  /**
   * The height in meters (above the ellipsoid) of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Height = 0.0;

  /**
   * The Earth-Centered Earth-Fixed X-coordinate of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_X = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Y-coordinate of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_Y = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Z-coordinate of this component.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_Z = 0.0;

  /**
   * Aligns the local up direction with the ellipsoid normal at the current
   * location.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cesium")
  void SnapLocalUpToEllipsoidNormal();

  /**
   * Turns the actor's local coordinate system into a East-South-Up tangent
   * space in centimeters.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cesium")
  void SnapToEastSouthUp();

  /**
   * Move the actor to the specified longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   */
  void MoveToLongitudeLatitudeHeight(
      const glm::dvec3& TargetLongitudeLatitudeHeight,
      bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateMoveToLongitudeLatitudeHeight(
      const FVector& TargetLongitudeLatitudeHeight,
      bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified Earth-Centered, Earth-Fixed (ECEF)
   * coordinates.
   */
  void MoveToECEF(
      const glm::dvec3& TargetEcef,
      bool MaintainRelativeOrientation = true);

  /**
   * Move the actor to the specified Earth-Centered, Earth-Fixed (ECEF)
   * coordinates.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateMoveToECEF(
      const FVector& TargetEcef,
      bool MaintainRelativeOrientation = true);

  // TODO Comment (or rather remove it - only used in CesiumGlobeAnchorParent!)
  void SetAutoSnapToEastSouthUp(bool bValue);

  /**
   * Called by owner actor on position shifting. The Component should update
   * all relevant data structures to reflect new actor location.
   *
   * Specifically, this is called during origin rebasing. Here, it
   * will update the ECEF location, to take the new world origin
   * location into account.
   */
  virtual void
  ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

protected:
  /**
   * Called when a component is registered.
   *
   * In fact, this is called on many other occasions (including
   * changes to properties in the editor). Here, it us used to
   * attach the callback to `HandleActorTransformUpdated` to
   * the `TransformUpdated` delegate of the owner root, so that
   * movements in the actor can be used for updating the
   * ECEF coordinates.
   */
  virtual void OnRegister() override;

  /**
   * Called when a component is unregistered.
   *
   * In fact, this is called on many other occasions (including
   * changes to properties in the editor). Here, it us used to
   * DEtach the callback to `HandleActorTransformUpdated` from
   * the `TransformUpdated` delegate of the owner root.
   */
  virtual void OnUnregister() override;

  /**
   * Handle updates in the transform of the owning actor.
   *
   * This will be attached to the `TransformUpdated` delegate of
   * the owner root, and just call `_updateFromActor`, to
   * include the new actor transform in the ECEF coordinates.
   */
  void HandleActorTransformUpdated(
      USceneComponent* InRootComponent,
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport);

  /**
   * Called when a component is created (not loaded).
   * This can happen in the editor or during gameplay.
   *
   * This is overrideen for initializing the Georeference by calling
   * _initGeoreference. This indeed seems to be called only exactly
   * once, when the component is created in the editor.
   */
  virtual void OnComponentCreated() override;

  /**
   * Do any object-specific cleanup required immediately after
   * loading an object.
   *
   * This is overrideen for initializing the Georeference by calling
   * _initGeoreference. This indeed seems to  be called only exactly
   * once, when the component is loaded as part of a level in the editor.
   */
  virtual void PostLoad() override;

#if WITH_EDITOR

  /**
   * This is called when a property is about to be modified externally.
   *
   * When the georeference is about to be modified, then this will
   * remove the `HandleGeoreferenceUpdated` callback from the
   * `OnGeoreferenceUpdated` delegate of the current georeference.
   */
  void PreEditChange(FProperty* PropertyThatWillChange);

  /**
   * Called when a property on this object has been modified externally
   *
   * This is called every time that a value is modified in the editor UI.
   *
   * When a cartographic value is modified, calls
   * `MoveToLongitudeLatitudeHeight` with the new values.
   *
   * When an ECEF coordinate is modified, it calls `MoveToECEF` with the
   * new values.
   *
   * When the georeference is about to be modified, then this will
   * attach the `HandleGeoreferenceUpdated` callback to the
   * `OnGeoreferenceUpdated` delegate of the new georeference,
   * and call `_updateActorTransform` to take the new georeference
   * into account.
   */
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
  /**
   * A function that is attached to the `OnGeoreferenceUpdated` delegate,
   * of the Georeference, and just calls `_updateActorTransform`.
   */
  UFUNCTION()
  void HandleGeoreferenceUpdated();

  /**
   * Initializes the `Georeference`.
   *
   * If there is no `Georeference`, then a default one is obtained.
   * In any case, `HandleGeoreferenceUpdated` callback will be attached
   * to the `OnGeoreferenceChanged` delegate, to be notified about
   * changes in the georeference that may affect this component.
   */
  void _initGeoreference();

  /**
   * Updates the ECEF coordinates of this instance, based on an update
   * of the transform of the owning actor.
   *
   * This will compute the new ECEF coordinates, based on the actor
   * transform, and assign them to this instance by calling `_setECEF`.
   */
  void _updateFromActor();

  /**
   * Obtains the absolute location of the owner actor.
   *
   * This is the sum of the world origin location and the relative location
   * (as of `this->GetOwner()->GetRootComponent()->GetComponentLocation()`),
   * as a GLM vector, with the appropriate validity checks.
   */
  glm::dvec3 _getAbsoluteLocationFromActor();

  /**
   * Obtains the current rotation matrix from the transform of the actor.
   */
  glm::dmat3 _getRotationFromActor();

  /**
   * Set the position of this component, in ECEF coordinates.
   *
   * This will perform the necessary updates of the `ECEF_X`, `ECEF_Y`,
   * and `ECEF_Z` properties as well as the cartographic coordinate
   * properties, and call `_updateActorTransform`.
   *
   * @param targetEcef The ECEF coordinates.
   * @param maintainRelativeOrientation TODO Explain that...
   */
  void _setECEF(const glm::dvec3& targetEcef, bool maintainRelativeOrientation);

  /**
   * Computes the relative location, from the current ECEF location
   * and the world origin location.
   */
  glm::dvec3 _computeRelativeLocation();

  /**
   * Updates the transform of the owning actor.
   *
   * This will use the (high-precision) `ECEF_X`, `ECEF_Y`, and `ECEF_Z`
   * coordinates of this instance, as well as the rotation component of
   * the current actor transform, to compute an updated transform matrix
   * that is applied to the owner by calling `SetWorldTransform`.
   */
  void _updateActorTransform();

  /**
   * Updates the transform of the owning actor.
   *
   * @param rotation The rotation component
   * @param translation The translation component
   */
  void _updateActorTransform(
      const glm::dmat3& rotation,
      const glm::dvec3& translation);

  /**
   * Updates the `Longitude`, `Latitude` and `Height` properties
   * of this instance, based on the current ECEF_X/Y/Z coordinates.
   */
  void _updateDisplayLongitudeLatitudeHeight();

  /**
   * A function to print some debug message about the state
   * (absolute and relative location) of this component.
   */
  void _debugLogState();

  /**
   * Whether an update of the actor transform is currently in progress,
   * and further calls that are issued by HandleActorTransformUpdated
   * should be ignored
   */
  bool _updatingActorTransform;

  // TODO GEOREF_REFACTORING
  // This is currently no longer used, but removing this messes up
  // the deserialization (unless we handle different versions)
  // Note: this is done to allow Unreal to recognize and serialize _actorToECEF
  UPROPERTY()
  double _actorToECEF_Array[16];
  glm::dmat4& _actorToECEF = *(glm::dmat4*)_actorToECEF_Array;

  UPROPERTY()
  bool _autoSnapToEastSouthUp;
};
