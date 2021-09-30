// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>
#include "CesiumGlobeAnchorComponent.generated.h"

class ACesiumGeoreference;

/**
 * This component can be added to a movable actor to anchor it to the globe
 * and maintain precise placement. When the owning actor is transformed through
 * normal Unreal Engine mechanisms, the internal geospatial coordinates will be
 * automatically updated. The actor position can also be set in terms of
 * Earth-Centered, Eath-Fixed coordinates (ECEF) or Longitude, Latitude, and
 * Height relative to the ellipsoid.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGlobeAnchorComponent : public UActorComponent {
  GENERATED_BODY()

private:
  /**
   * The designated georeference actor controlling how the owning actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the Component will find and use the first Georeference
   * Actor in the level, or create one if necessary. To get the active/effective
   * Georeference from Blueprints or C++, use ResolvedGeoreference instead.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetGeoreference,
      BlueprintSetter = SetGeoreference,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  ACesiumGeoreference* Georeference = nullptr;

  /**
   * The resolved georeference used by this component. This is not serialized
   * because it may point to a Georeference in the PersistentLevel while this
   * component is in a sublevel. If the Georeference property is specified,
   * however then this property will have the same value.
   *
   * This property will be null before ResolveGeoreference is called, which
   * happens automatically when the component is registered.
   */
  UPROPERTY(Transient, BlueprintReadOnly, Meta = (AllowPrivateAccess))
  ACesiumGeoreference* ResolvedGeoreference = nullptr;

public:
  /** @copydoc UCesiumGlobeAnchorComponent::Georeference */
  UFUNCTION(BlueprintCallable)
  ACesiumGeoreference* GetGeoreference() const;

  /** @copydoc UCesiumGlobeAnchorComponent::Georeference */
  UFUNCTION(BlueprintCallable)
  void SetGeoreference(ACesiumGeoreference* NewGeoreference);

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
   * setting it to null. The next time ResolveGeoreference is called, the
   * Georeference will be re-resolved and re-subscribed.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InvalidateResolvedGeoreference();

private:
  /**
   * The latitude in degrees of this component, in the range [-90, 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta = (AllowPrivateAccess, ClampMin = -90.0, ClampMax = 90.0))
  double Latitude = 0.0;

  /**
   * The longitude in degrees of this component, in the range [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (AllowPrivateAccess, ClampMin = -180.0, ClampMax = 180.0))
  double Longitude = 0.0;

  /**
   * The height in meters above the ellipsoid (usually WGS84) of this component.
   * Do not confuse this with a geoid height or height above mean sea level,
   * which can be tens of meters higher or lower depending on where in the world
   * the object is located.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (AllowPrivateAccess))
  double Height = 0.0;

public:
  /**
   * Returns the longitude in degrees (x), latitude in degrees (y),
   * and height in meters (z) of the actor.
   *
   * Returns a zero vector if the component is not yet registered.
   */
  glm::dvec3 GetLongitudeLatitudeHeight() const;

  /**
   * Returns the longitude in degrees (X), latitude in degrees (Y),
   * and height in meters (Z) of the actor.
   *
   * This function is inaccurate because large coordinate values are represented
   * as singe-precision floating point numbers.
   *
   * Returns a zero vector if the component is not yet registered.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateGetLongitudeLatitudeHeight() const;

  /**
   * Move the actor to the specified longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   *
   * If `AdjustOrientationForGlobeWhenMoving` is enabled, the Actor's
   * orientation will also be adjusted to account for globe curvature.
   */
  void MoveToLongitudeLatitudeHeight(
      const glm::dvec3& TargetLongitudeLatitudeHeight);

  /**
   * Move the actor to the specified longitude in degrees (x), latitude
   * in degrees (y), and height in meters (z).
   *
   * If `AdjustOrientationForGlobeWhenMoving` is enabled, the Actor's
   * orientation will also be adjusted to account for globe curvature.
   *
   * This function is inaccurate because large coordinate values are represented
   * as singe-precision floating point numbers.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateMoveToLongitudeLatitudeHeight(
      const FVector& TargetLongitudeLatitudeHeight);

private:
  /**
   * The Earth-Centered Earth-Fixed X-coordinate of this component in meters.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (AllowPrivateAccess))
  double ECEF_X = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Y-coordinate of this component in meters.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (AllowPrivateAccess))
  double ECEF_Y = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Z-coordinate of this component in meters.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (AllowPrivateAccess))
  double ECEF_Z = 0.0;

public:
  /**
   * Returns the Earth-Centered, Earth-Fixed (ECEF) coordinates of the actor in
   * meters.
   *
   * Returns a zero vector if the component is not yet registered.
   */
  glm::dvec3 GetECEF() const;

  /**
   * Returns the Earth-Centered, Earth-Fixed (ECEF) coordinates of the actor in
   * meters, downcasted to a single-precision floating point vector.
   *
   * Returns a zero vector if the component is not yet registered.
   *
   * This function is inaccurate because large coordinate values are represented
   * as singe-precision floating point numbers.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateGetECEF() const;

  /**
   * Moves the Actor to which this component is attached to a given globe
   * position in Earth-Centered, Earth-Fixed coordinates in meters.
   *
   * If AdjustOrientationForGlobeWhenMoving is enabled, this method will also
   * update the orientation based on the globe curvature.
   *
   * @param newPosition The new position.
   */
  void MoveToECEF(const glm::dvec3& newPosition);

  /**
   * Moves the Actor to which this component is attached to a given globe
   * position in Earth-Centered, Earth-Fixed coordinates in meters.
   *
   * If AdjustOrientationForGlobeWhenMoving is enabled, this method will also
   * update the orientation based on the globe curvature.
   *
   * This function is inaccurate because large coordinate values are represented
   * as singe-precision floating point numbers.
   *
   * @param newPosition The new position.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateMoveToECEF(const FVector& TargetEcef);

  /**
   * Rotates the Actor so that its local +Z axis is aligned with the ellipsoid
   * surface normal at its current location.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cesium")
  void SnapLocalUpToEllipsoidNormal();

  /**
   * Rotates the Actor so that its +X axis points in the local East direction,
   * its +Y axis points in the local South direction, and its +Z axis points in
   * the local Up direction.
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cesium")
  void SnapToEastSouthUp();

public:
  /**
   * Using the teleport flag will move objects to the updated transform
   * immediately and without affecting their velocity. This is useful when
   * working with physics actors that maintain an internal velocity which we do
   * not want to change when updating location.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool TeleportWhenUpdatingTransform = true;

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
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool AdjustOrientationForGlobeWhenMoving = true;

  //
  // Base class overrides
  //

  /**
   * Called by the owner actor when the world's OriginLocation changes (i.e.
   * during origin rebasing). The Component will recompute the Actor's
   * transform based on the new OriginLocation and on this component's
   * globe transform. The Actor's orientation is unaffected.
   */
  virtual void
  ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

  /**
   * Handles reading, writing, and reference collecting using FArchive.
   * This implementation handles all FProperty serialization, but can be
   * overridden for native variables.
   *
   * This class overrides this method to ensure internal variables are
   * immediately synchronized with newly-loaded values.
   */
  virtual void Serialize(FArchive& Ar) override;

  /**
   * Called when a component is created (not loaded). This can happen in the
   * editor or during gameplay.
   *
   * This method is invoked after this component is pasted and just prior to
   * registration. We mark the globe transform invalid here because we can't
   * assume the globe transform is still valid when the component is pasted into
   * another Actor, or even if the Actor was changed since the Component was
   * copied.
   */
  virtual void OnComponentCreated() override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
  /**
   * Called when a component is registered. This can be viewed as "enabling"
   * this Component on the Actor to which it is attached.
   *
   * In the Editor, this is called in a many different situations, such as on
   * changes to properties.
   */
  virtual void OnRegister() override;

  /**
   * Called when a component is unregistered. This can be viewed as
   * "disabling" this Component on the Actor to which it is attached.
   *
   * In the Editor, this is called in a many different situations, such as on
   * changes to properties.
   */
  virtual void OnUnregister() override;

private:
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
  UPROPERTY()
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
