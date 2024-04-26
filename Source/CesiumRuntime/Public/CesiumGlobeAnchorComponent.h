// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeospatial/GlobeAnchor.h"
#include "Components/ActorComponent.h"
#include "Delegates/IDelegateInstance.h"
#include "CesiumGlobeAnchorComponent.generated.h"

class ACesiumGeoreference;

/**
 * This component can be added to a movable actor to anchor it to the globe
 * and maintain precise placement. When the owning actor is transformed through
 * normal Unreal Engine mechanisms, the internal geospatial coordinates will be
 * automatically updated. The actor position can also be set in terms of
 * Earth-Centered, Earth-Fixed coordinates (ECEF) or Longitude, Latitude, and
 * Height relative to the ellipsoid.
 */
UCLASS(ClassGroup = Cesium, Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGlobeAnchorComponent : public UActorComponent {
  GENERATED_BODY()

#pragma region Properties
private:
  /**
   * The designated georeference actor controlling how the owning actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the Component will find and use the first Georeference
   * Actor in the level, or create one if necessary. To get the active/effective
   * Georeference from Blueprints or C++, use ResolvedGeoreference instead.
   *
   * If setting this property changes the CesiumGeoreference, the globe position
   * will be maintained and the Actor's transform will be updated according to
   * the new CesiumGeoreference.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetGeoreference,
      BlueprintSetter = SetGeoreference,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  TSoftObjectPtr<ACesiumGeoreference> Georeference = nullptr;

  /**
   * The resolved georeference used by this component. This is not serialized
   * because it may point to a Georeference in the PersistentLevel while this
   * component is in a sub-level. If the Georeference property is specified,
   * however then this property will have the same value.
   *
   * This property will be null before ResolveGeoreference is called, which
   * happens automatically when the component is registered.
   */
  UPROPERTY(
      Transient,
      VisibleAnywhere,
      BlueprintReadOnly,
      BlueprintGetter = GetResolvedGeoreference,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  ACesiumGeoreference* ResolvedGeoreference = nullptr;

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
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetAdjustOrientationForGlobeWhenMoving,
      BlueprintSetter = SetAdjustOrientationForGlobeWhenMoving,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  bool AdjustOrientationForGlobeWhenMoving = true;

  /**
   * Using the teleport flag will move objects to the updated transform
   * immediately and without affecting their velocity. This is useful when
   * working with physics actors that maintain an internal velocity which we do
   * not want to change when updating location.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetTeleportWhenUpdatingTransform,
      BlueprintSetter = SetTeleportWhenUpdatingTransform,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  bool TeleportWhenUpdatingTransform = true;

  /**
   * The 4x4 transformation matrix from the Actors's local coordinate system to
   * the Earth-Centered, Earth-Fixed (ECEF) coordinate system.
   *
   * The ECEF coordinate system is a right-handed system located at the center
   * of the Earth. The +X axis points to the intersection of the Equator and
   * Prime Meridian (zero degrees longitude). The +Y axis points to the
   * intersection of the Equator and +90 degrees longitude. The +Z axis points
   * up through the North Pole.
   *
   * If `AdjustOrientationForGlobeWhenMoving` is enabled and this property is
   * set, the Actor's orientation will also be adjusted to account for globe
   * curvature.
   */
  UPROPERTY(
      BlueprintReadWrite,
      BlueprintGetter = GetActorToEarthCenteredEarthFixedMatrix,
      BlueprintSetter = SetActorToEarthCenteredEarthFixedMatrix,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  FMatrix ActorToEarthCenteredEarthFixedMatrix;

#pragma endregion

#pragma region Property Accessors
public:
  /**
   * Gets the designated georeference actor controlling how the owning actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the Component will find and use the first Georeference
   * Actor in the level, or create one if necessary. To get the active/effective
   * Georeference from Blueprints or C++, use ResolvedGeoreference instead.
   */
  UFUNCTION(BlueprintGetter)
  TSoftObjectPtr<ACesiumGeoreference> GetGeoreference() const;

  /**
   * Sets the designated georeference actor controlling how the owning actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the Component will find and use the first Georeference
   * Actor in the level, or create one if necessary. To get the active/effective
   * Georeference from Blueprints or C++, use ResolvedGeoreference instead.
   */
  UFUNCTION(BlueprintSetter)
  void SetGeoreference(TSoftObjectPtr<ACesiumGeoreference> NewGeoreference);

  /**
   * Gets the resolved georeference used by this component. This is not
   * serialized because it may point to a Georeference in the PersistentLevel
   * while this component is in a sub-level. If the Georeference property is
   * manually specified, however, then this property will have the same value.
   *
   * This property will be null before ResolveGeoreference is called, which
   * happens automatically when the component is registered.
   */
  UFUNCTION(BlueprintGetter)
  ACesiumGeoreference* GetResolvedGeoreference() const;

  /**
   * Resolves the Cesium Georeference to use with this Component. Returns
   * the value of the Georeference property if it is set. Otherwise, finds a
   * Georeference in the World and returns it, creating it if necessary. The
   * resolved Georeference is cached so subsequent calls to this function will
   * return the same instance, unless ForceReresolve is true.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  ACesiumGeoreference* ResolveGeoreference(bool bForceReresolve = false);

  /**
   * Gets the 4x4 transformation matrix from the Actors's local coordinate
   * system to the Earth-Centered, Earth-Fixed (ECEF) coordinate system.
   *
   * The ECEF coordinate system is a right-handed system located at the center
   * of the Earth. The +X axis points to the intersection of the Equator and
   * Prime Meridian (zero degrees longitude). The +Y axis points to the
   * intersection of the Equator and +90 degrees longitude. The +Z axis points
   * up through the North Pole.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  FMatrix GetActorToEarthCenteredEarthFixedMatrix() const;

  /**
   * Sets the 4x4 transformation matrix from the Actors's local coordinate
   * system to the Earth-Centered, Earth-Fixed (ECEF) coordinate system.
   *
   * The ECEF coordinate system is a right-handed system located at the center
   * of the Earth. The +X axis points to the intersection of the Equator and
   * Prime Meridian (zero degrees longitude). The +Y axis points to the
   * intersection of the Equator and +90 degrees longitude. The +Z axis points
   * up through the North Pole.
   *
   * If `AdjustOrientationForGlobeWhenMoving` is enabled, the Actor's
   * orientation will also be adjusted to account for globe curvature.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetActorToEarthCenteredEarthFixedMatrix(const FMatrix& Value);

  /**
   * Gets a flag indicating whether to move objects to the updated transform
   * immediately and without affecting their velocity. This is useful when
   * working with physics actors that maintain an internal velocity which we do
   * not want to change when updating location.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  bool GetTeleportWhenUpdatingTransform() const;

  /**
   * Sets a flag indicating whether to move objects to the updated transform
   * immediately and without affecting their velocity. This is useful when
   * working with physics actors that maintain an internal velocity which we do
   * not want to change when updating location.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetTeleportWhenUpdatingTransform(bool Value);

  /**
   * Gets a flag indicating whether to adjust the Actor's orientation based on
   * globe curvature as the Actor moves.
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
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  bool GetAdjustOrientationForGlobeWhenMoving() const;

  /**
   * Sets a flag indicating whether to adjust the Actor's orientation based on
   * globe curvature as the Actor moves.
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
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetAdjustOrientationForGlobeWhenMoving(bool Value);

#pragma endregion

#pragma region Public Methods

public:
  /**
   * Gets the longitude in degrees (X), latitude in degrees (Y),
   * and height in meters above the ellipsoid (Z) of the actor.
   *
   * Do not confuse the ellipsoid height with a geoid height or height above
   * mean sea level, which can be tens of meters higher or lower depending on
   * where in the world the object is located.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      Meta = (ReturnDisplayName = "LongitudeLatitudeHeight"))
  FVector GetLongitudeLatitudeHeight() const;

  /**
   * Gets the longitude in degrees.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      Meta = (ReturnDisplayName = "Longitude"))
  double GetLongitude() const { return this->GetLongitudeLatitudeHeight().X; }

  /**
   * Gets the latitude in degrees.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      Meta = (ReturnDisplayName = "Latitude"))
  double GetLatitude() const { return this->GetLongitudeLatitudeHeight().Y; }

  /**
   * Gets the height in meters above the ellipsoid.
   *
   * Do not confuse the ellipsoid height with a geoid height or height above
   * mean sea level, which can be tens of meters higher or lower depending on
   * where in the world the object is located.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      Meta = (ReturnDisplayName = "Height"))
  double GetHeight() const { return this->GetLongitudeLatitudeHeight().Z; }

  /**
   * Moves the Actor to which this component is attached to a given longitude in
   * degrees (X), latitude in degrees (Y), and height in meters (Z).
   *
   * The Height (Z) is measured in meters above the WGS84 ellipsoid. Do not
   * confused an ellipsoidal height with a geoid height or height above mean sea
   * level, which can be tens of meters higher or lower depending on where in
   * the world the object is located.
   *
   * If `AdjustOrientationForGlobeWhenMoving` is enabled, the Actor's
   * orientation will also be adjusted to account for globe curvature.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void MoveToLongitudeLatitudeHeight(const FVector& LongitudeLatitudeHeight);

  /**
   * Gets the Earth-Centered, Earth-Fixed (ECEF) coordinates of the Actor in
   * meters.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EarthCenteredEarthFixedPosition"))
  FVector GetEarthCenteredEarthFixedPosition() const;

  /**
   * Moves the Actor to which this component is attached to a given globe
   * position in Earth-Centered, Earth-Fixed coordinates in meters.
   *
   * If AdjustOrientationForGlobeWhenMoving is enabled, this method will
   * also update the orientation based on the globe curvature.
   *
   * @param newPosition The new position.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void MoveToEarthCenteredEarthFixedPosition(
      const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Gets the rotation of the Actor relative to a local coordinate system
   * centered on this object where the +X points in the local East direction,
   * the +Y axis points in the local South direction, and the +Z axis points in
   * the local Up direction.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EastSouthUpRotation"))
  FQuat GetEastSouthUpRotation() const;

  /**
   * Sets the rotation of the Actor relative to a local coordinate system
   * centered on this object where the +X points in the local East direction,
   * the +Y axis points in the local South direction, and the +Z axis points in
   * the local Up direction.
   *
   * When the rotation is set via this method, it is internally converted to
   * and stored in the ActorToEarthCenteredEarthFixedMatrix property. As a
   * result, getting this property will not necessarily return the exact value
   * that was set.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetEastSouthUpRotation(const FQuat& EastSouthUpRotation);

  /**
   * Gets the rotation of the Actor relative to the Earth-Centered, Earth-Fixed
   * (ECEF) coordinate system.
   *
   * The ECEF coordinate system is a right-handed system located at the center
   * of the Earth. The +X axis points from there to the intersection of the
   * Equator and Prime Meridian (zero degrees longitude). The +Y axis points to
   * the intersection of the Equator and +90 degrees longitude. The +Z axis
   * points up through the North Pole.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EarthCenteredEarthFixedRotation"))
  FQuat GetEarthCenteredEarthFixedRotation() const;

  /**
   * Sets the rotation of the Actor relative to the Earth-Centered, Earth-Fixed
   * (ECEF) coordinate system.
   *
   * The ECEF coordinate system is a right-handed system located at the center
   * of the Earth. The +X axis points from there to the intersection of the
   * Equator and Prime Meridian (zero degrees longitude). The +Y axis points to
   * the intersection of the Equator and +90 degrees longitude. The +Z axis
   * points up through the North Pole.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetEarthCenteredEarthFixedRotation(
      const FQuat& EarthCenteredEarthFixedRotation);

  /**
   * Rotates the Actor so that its local +Z axis is aligned with the ellipsoid
   * surface normal at its current location.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SnapLocalUpToEllipsoidNormal();

  /**
   * Rotates the Actor so that its +X axis points in the local East direction,
   * its +Y axis points in the local South direction, and its +Z axis points in
   * the local Up direction.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SnapToEastSouthUp();

  /**
   * Synchronizes the properties of this globe anchor.
   *
   * It is usually not necessary to call this method because it is called
   * automatically when needed.
   *
   * This method performs the following actions:
   *
   *   - If the ActorToEarthCenteredEarthFixedMatrix has not yet been
   * determined, it is computed from the Actor's current root transform.
   *   - If the Actor's root transform has changed since the last time this
   * component was registered, this method updates the
   * ActorToEarthCenteredEarthFixedMatrix from the current transform.
   *   - If the origin of the CesiumGeoreference has changed, the Actor's root
   * transform is updated based on the ActorToEarthCenteredEarthFixedMatrix and
   * the new georeference origin.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void Sync();

#pragma endregion

#pragma region Obsolete
public:
  /**
   * @brief DEPRECATED
   * @deprecated The resolved georeference can no longer be explicitly
   * invalidated. To change the georeference, call SetGeoreference or
   * ReregisterComponent.
   */
  UE_DEPRECATED(
      "Cesium For Unreal v2.0",
      "The resolved georeference can no longer be explicitly invalidated. To change the georeference, call SetGeoreference or ReregisterComponent.")
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "The resolved georeference can no longer be explicitly invalidated. To change the georeference, call SetGeoreference or ReregisterComponent."))
  void InvalidateResolvedGeoreference();
#pragma endregion

#pragma region Unreal Lifecycle
protected:
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
#pragma endregion

#pragma region Implementation Details
private:
  CesiumGeospatial::GlobeAnchor _createNativeGlobeAnchor() const;

  USceneComponent* _getRootComponent(bool warnIfNull) const;

  FTransform _getCurrentRelativeTransform() const;

  void _setCurrentRelativeTransform(const FTransform& relativeTransform);

  CesiumGeospatial::GlobeAnchor
  _createOrUpdateNativeGlobeAnchorFromRelativeTransform(
      const FTransform& newRelativeTransform);

  CesiumGeospatial::GlobeAnchor
  _createOrUpdateNativeGlobeAnchorFromECEF(const FMatrix& newActorToECEFMatrix);

  void _updateFromNativeGlobeAnchor(
      const CesiumGeospatial::GlobeAnchor& nativeAnchor);

  void _setNewActorToECEFFromRelativeTransform();

#if WITH_EDITORONLY_DATA
  // This is used only to preserve the transformation saved by old versions of
  // Cesium for Unreal. See the Serialize method.
  UPROPERTY(Meta = (DeprecatedProperty))
  double _actorToECEF_Array_DEPRECATED[16];
#endif

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

  bool _lastRelativeTransformIsValid = false;
  FTransform _lastRelativeTransform{};

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

  friend class FCesiumGlobeAnchorCustomization;
#pragma endregion
};
