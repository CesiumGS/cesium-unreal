// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>
#include "CesiumGlobeAnchorComponent.generated.h"

class ACesiumGeoreference;

/**
 * This component can be added to movable actors to globally georeference them
 * and maintain precise placement. When the owning actor is transformed through
 * normal Unreal Engine mechanisms, the internal geospatial coordinates will be
 * automatically updated. The actor position can also be set in terms of
 * Earth-Centered, Eath-Fixed coordinates (ECEF) or Longitude, Latitude, and
 * Height relative to the ellipsoid.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGlobeAnchorComponent : public UActorComponent {
  GENERATED_BODY()

public:
  /**
   * The georeference actor controlling how the owning actor's coordinate system
   * relates to the coordinate system in this Unreal Engine level.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ACesiumGeoreference* Georeference = nullptr;

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
   * Whether to adjust the Actor's orientation based on globe curvature as the
   * Actor moves.
   *
   * The Earth is not flat, so as we move across its surface, the direction of
   * "up" changes. If we ignore this fact and leave an object's orientation
   * unchanged as it moves over the globe surface, the object will become
   * increasingly tilted and eventually be completely upside-down when we arrive
   * at the opposite side of the globe.
   *
   * When this setting is enabled, this Component will automatically apply a
   * rotation to the Actor to account for globe curvature any time the Actor's
   * position on the globe changes.
   *
   * This property should usually be enabled, but it may be useful to disable it
   * when your application already accounts for globe curvature itself when it
   * updates an Actor's position and orientation, because in that case the Actor
   * would be over-rotated.
   */
  bool AdjustOrientationForGlobeWhenMoving = true;

  /**
   * Resolves the Cesium Georeference to use with this Component. Returns
   * the value of the Georeference property if it is set. Otherwise, finds a
   * Georeference in the World and returns it, creating it if necessary. The
   * resolved Georeference is cached so subsequent calls to this function will
   * return the same instance.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  ACesiumGeoreference* ResolveGeoreference();

  /**
   * Invalidates the cached resolved georeference, unsubscribing from it and
   * setting it to null. The next time GetResolvedGeoreference is called, the
   * Georeference will be re-resolved and re-subscribed.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InvalidateResolvedGeoreference();

  /**
   * Called by the owner actor when the world's OriginLocation changes (i.e.
   * during origin rebasing). The Component will recompute the Actor's
   * transform based on the new OriginLocation and on this component's
   * globe transform. The Actor's orientation is unaffected.
   */
  virtual void
  ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
  /**
   * Called when a component is registered. This can be viewed as "activating"
   * this Component on the Actor to which it is attached.
   *
   * In the Editor, this is called in a many different situations, such as on
   * changes to properties.
   */
  virtual void OnRegister() override;

  /**
   * Called when a component is unregistered. This can be viewed as
   * "deactivating" this Component on the Actor to which it is attached.
   *
   * In the Editor, this is called in a many different situations, such as on
   * changes to properties.
   */
  virtual void OnUnregister() override;

private:
  /**
   * The resolved georeference used by this component. This is not serialized
   * because it may point to a Georeference in the PersistentLevel while this
   * tileset is in a sublevel. If the Georeference property is specified,
   * however then this property will have the same value.
   *
   * Use `ResolveGeoreference` rather than accessing this directly.
   */
  UPROPERTY(Transient)
  ACesiumGeoreference* _pResolvedGeoreference = nullptr;

  /**
   * The current Actor to ECEF transformation expressed as a simple array of
   * doubles so that Unreal Engine can serialize it.
   */
  UPROPERTY()
  double _actorToECEF_Array[16];

  static_assert(sizeof(_actorToECEF_Array) == sizeof(glm::dmat4));

  /**
   * The _actorToECEF_Array cast as a glm::dmat4 for convenience.
   */
  glm::dmat4& _actorToECEF = *reinterpret_cast<glm::dmat4*>(_actorToECEF_Array);

  /**
   * True if the globe transform is a valid and correct representation of the
   * position and orientation of this Actor. False if the globe transform has
   * not yet been computed and so the Actor transform is the only valid
   * representation of the Actor's position and orientation.
   */
  bool _actorToECEFIsValid = false;

  /**
   * Whether an update of the actor transform is currently in progress,
   * and further calls that are received by _onActorTransformChanged
   * should be ignored
   */
  bool _updatingActorTransform = false;

  /**
   * Called when the root transform of the Actor to which this Component is
   * attached has changed. So:
   *  * The globe (ECEF) position and orientation are computed from the new
   * transform.
   *  * When `AdjustOrientationForGlobeWhenMoving` is enabled, the orientation
   * will also be adjusted for globe curvature.
   */
  void _onActorTransformChanged(
      USceneComponent* InRootComponent,
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport);

  /**
   * Called when the Component switches to a new Georeference Actor or the
   * existing Georeference is given a new origin Longitude, Latitude, or
   * Height. The Actor's position and orientation are recomputed from the
   * Component's globe (ECEF) position and orientation.
   */
  UFUNCTION()
  void _onGeoreferenceChanged();

  /**
   * Updates the globe-relative (ECEF) transform from the current Actor
   * transform.
   *
   * @returns The new globe position.
   */
  const glm::dmat4& _updateGlobeTransformFromActorTransform();

  /**
   * Updates the Unreal world Actor position from the current globe position.
   *
   * @param newWorldOrigin The new world OriginLocation to use when computing
   * the Actor transform. If std::nullopt, the true world OriginLocation is
   * used.
   * @return The new Actor position.
   */
  FTransform _updateActorTransformFromGlobeTransform(
      const std::optional<glm::dvec3>& newWorldOrigin = std::nullopt);

  /**
   * Sets a new globe transform and updates the Actor transform to match. If
   * `AdjustOrientationForGlobeWhenMoving` is enabled, the orientation is also
   * adjusted for globe curvature.
   *
   * This function does not update the Longitude, Latitude, Height, ECEF_X,
   * ECEF_Y, or ECEF_Z properties. To do that, call `_updateCartesianProperties`
   * and `_updateCartographicProperties`.
   *
   * @param newTransform The new transform, before it is adjusted for globe
   * curvature.
   * @returns The new globe transform, which may be different from
   * `newTransform` if the orientation has been adjusted for globe curvature.
   */
  const glm::dmat4& _setGlobeTransform(const glm::dmat4& newTransform);

  /**
   * Applies the current values of the ECEF_X, ECEF_Y, and ECEF_Z properties,
   * updating the Longitude, Latitude, and Height properties, the globe
   * transform, and the Actor transform. If
   * `AdjustOrientationForGlobeWhenMoving` is enabled, the orientation is also
   * adjusted for globe curvature.
   */
  void _applyCartesianProperties();

  /**
   * Updates the ECEF_X, ECEF_Y, and ECEF_Z properties from the current globe
   * transform.
   */
  void _updateCartesianProperties();

  /**
   * Applies the current values of the Longitude, Latitude, and Height
   * properties, updating the ECEF_X, ECEF_Y, and ECEF_Z properties, the globe
   * transform, and the Actor transform. If
   * `AdjustOrientationForGlobeWhenMoving` is enabled, the orientation is also
   * adjusted for globe curvature.
   */
  void _applyCartographicProperties();

  /**
   * Updates the Longitude, Latitude, and Height properties from the current
   * globe transform.
   */
  void _updateCartographicProperties();
};
