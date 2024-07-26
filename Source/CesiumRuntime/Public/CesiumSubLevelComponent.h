// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include "UObject/ObjectMacros.h"
#include "CesiumSubLevelComponent.generated.h"

class ACesiumGeoreference;
class ALevelInstance;
class UCesiumSubLevelSwitcherComponent;

/**
 * A component intended to be attached to a Level Instance Actor that turns that
 * Level Instance into a Cesium sub-level. Only a single Cesium sub-level can be
 * active (visible) at any given time.
 *
 * A globe (like the planet Earth) is an unusual sort of level in Unreal in that
 * it has truly massive coordinate values and the "up" direction depends on
 * where exactly on the globe you're located. Many things in the Unreal
 * ecosystem, such as the gravity system, don't expect this situation and will
 * have incorrect and surprising behavior when used on a globe.
 *
 * Cesium sub-levels help to mitigate this. Only one sub-level can be active at
 * any given time, and when it is, that sub-level's origin becomes the origin of
 * the Unreal world. Furthermore, at the origin location, Unreal's +X axis
 * points East, its +Y axis points South, and its +Z axis points Up. Thus,
 * within a sub-level, gravity works in the normal way that Unreal objects
 * expect, and coordinate values stay relatively small. This allows you to use
 * just about any Unreal object within a sub-level without worrying about
 * surprising behavior.
 *
 * Globe-aware objects, particularly those with a "Cesium Globe Anchor"
 * component attached to them, are allowed to exist outside sub-levels and even
 * move between them. If all your objects are globe aware, there's no need to
 * use sub-levels at all.
 *
 * In the Editor, the currently-active sub-level is selected by clicking the
 * "Eye" icon next to the Level Instance in the Outliner.
 *
 * At runtime, the currently-active sub-level is selected by the Actor with a
 * CesiumOriginShiftComponent attached to it. If this Actor is inside a
 * sub-level's "Load Radius" that sub-level will be activated. If multiple
 * sub-levels are in range, only the closest one will be activated.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumSubLevelComponent : public UActorComponent {
  GENERATED_BODY()

public:
  /**
   * Gets whether this sub-level is enabled. An enabled sub-level will be
   * automatically loaded when the camera moves within its LoadRadius and
   * no other levels are closer, and the Georeference will be updated so that
   * this level's Longitude, Latitude, and Height become (0, 0, 0) in the Unreal
   * World. A sub-level that is not enabled will be ignored by Cesium at
   * runtime.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  bool GetEnabled() const;

  /**
   * Sets whether this sub-level is enabled. An enabled sub-level will be
   * automatically loaded when the camera moves within its LoadRadius and
   * no other levels are closer, and the Georeference will be updated so that
   * this level's Longitude, Latitude, and Height become (0, 0, 0) in the Unreal
   * World. A sub-level that is not enabled will be ignored by Cesium at
   * runtime.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetEnabled(bool value);

  /**
   * Gets the longitude of the georeference origin for this sub-level in
   * degrees, in the range [-180, 180]. When this sub-level is active, the
   * CesiumGeoreference will adopt this origin.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  double GetOriginLongitude() const;

  /**
   * Sets the longitude of the georeference origin for this sub-level in
   * degrees, in the range [-180, 180]. When this sub-level is active, the
   * CesiumGeoreference will adopt this origin.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetOriginLongitude(double value);

  /**
   * Gets the latitude of the georeference origin for this sub-level in degrees,
   * in the range [-90, 90]. When this sub-level is active, the
   * CesiumGeoreference will adopt this origin.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  double GetOriginLatitude() const;

  /**
   * Sets the latitude of the georeference origin for this sub-level in degrees,
   * in the range [-90, 90]. When this sub-level is active, the
   * CesiumGeoreference will adopt this origin.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetOriginLatitude(double value);

  /**
   * Gets the height of the georeference origin for this sub-level in meters
   * above the ellipsoid. This height should not be confused with a height
   * above Mean Sea Level. When this sub-level is active, the CesiumGeoreference
   * will adopt this origin.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  double GetOriginHeight() const;

  /**
   * Sets the height of the georeference origin for this sub-level in meters
   * above the ellipsoid. This height should not be confused with a height
   * above Mean Sea Level. When this sub-level is active, the CesiumGeoreference
   * will adopt this origin.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetOriginHeight(double value);

  /**
   * Gets how close to the sub-level local origin, in meters, the camera needs
   * to be to load the level.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  double GetLoadRadius() const;

  /**
   * Sets how close to the sub-level local origin, in meters, the camera needs
   * to be to load the level.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetLoadRadius(double value);

  /**
   * Gets the designated georeference actor controlling how the actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the sub-level will find and use the first Georeference
   * Actor in the level, or create one if necessary. To get the active/effective
   * Georeference from Blueprints or C++, use ResolvedGeoreference instead.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  TSoftObjectPtr<ACesiumGeoreference> GetGeoreference() const;

  /**
   * Sets the designated georeference actor controlling how the actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the sub-level will find and use the first Georeference
   * Actor in the level, or create one if necessary. To get the active/effective
   * Georeference from Blueprints or C++, use ResolvedGeoreference instead.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetGeoreference(TSoftObjectPtr<ACesiumGeoreference> NewGeoreference);

  /**
   * Gets the resolved georeference, just like calling the ResolveGeoreference
   * property, except that it will return null if a georeference has not yet
   * been resolved.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  ACesiumGeoreference* GetResolvedGeoreference() const;

  /**
   * Resolves the Cesium Georeference to use with this components. Returns
   * the value of the Georeference property if it is set. Otherwise, finds a
   * Georeference in the World and returns it, creating it if necessary. The
   * resolved Georeference is cached so subsequent calls to this function will
   * return the same instance, unless ForceReresolve is true.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  ACesiumGeoreference* ResolveGeoreference(bool bForceReresolve = false);

  /**
   * Sets the longitude (X), latitude (Y), and height (Z) of this sub-level's
   * georeference origin. When this sub-level is active, the CesiumGeoreference
   * will adopt this origin. Longitude and latitude are in degrees. Height is in
   * meters above the ellipsoid, which should not be confused with meters
   * above Mean Sea Level.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetOriginLongitudeLatitudeHeight(const FVector& longitudeLatitudeHeight);

#if WITH_EDITOR
  /**
   * Places the georeference origin at the origin of the sub-level and sets the
   * Level Instance's Location to (0,0,0). This improves the precision of the
   * objects in the sub-level as well as makes the Load Radius more sensible.
   *
   * If your sub-level has any Cesium3DTilesets, Cesium for Unreal will enter
   * Edit mode for the sub-level and the Cesium3DTilesets' transformations will
   * be updated based on the new georeference origin. You should Commit this
   * change.
   *
   * Warning: Before clicking, ensure that all non-Cesium objects in the
   * persistent level are georeferenced with the "CesiumGeoreferenceComponent"
   * or attached to an actor with that component. Ensure that static actors only
   * exist in georeferenced sub-levels.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void PlaceGeoreferenceOriginAtSubLevelOrigin();

  /**
   * Places the sub-level's origin at the camera's current location. Rotates
   * the globe so the current longitude/latitude/height of the camera is at the
   * Unreal origin of this sub-level. The camera is also teleported to the new
   * Unreal origin and rotated so that the view direction is maintained.
   *
   * This function is similar to "Place Georeference Origin Here" on the
   * CesiumGeoreference, except that this moves the georeference origin while
   * also ensuring that the sub-level content stays in the same place on the
   * globe by adjusting the Level Instance's transform.
   *
   * If your sub-level has any Cesium3DTilesets, Cesium for Unreal will enter
   * Edit mode for the sub-level and the Cesium3DTilesets' transformations will
   * be updated based on the new georeference origin. You should Commit this
   * change.
   *
   * Warning: Before clicking, ensure that all non-Cesium objects in the
   * persistent level are georeferenced with the "CesiumGlobeAnchorComponent"
   * or attached to an actor with that component. Ensure that static actors only
   * exist in georeferenced sub-levels.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void PlaceGeoreferenceOriginHere();
#endif

  /**
   * If this sub-level is currently the active one, this method will copy its
   * origin to the georeference's origin. Otherwise, it does nothing.
   */
  void UpdateGeoreferenceIfSubLevelIsActive();

  virtual void BeginDestroy() override;
  virtual void OnComponentCreated() override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
  virtual void BeginPlay() override;

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

#if WITH_EDITOR
  /**
   * Called by the Editor to check if it's ok to edit a property on this object.
   * Used to disable all fields on this component when editing the sub-level
   * instance that this component is attached to.
   */
  virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

private:
  /**
   * Whether this sub-level is enabled. An enabled sub-level will be
   * automatically loaded when the camera moves within its LoadRadius and
   * no other levels are closer, and the Georeference will be updated so that
   * this level's Longitude, Latitude, and Height become (0, 0, 0) in the Unreal
   * World. A sub-level that is not enabled will be ignored by Cesium at
   * runtime.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      BlueprintGetter = GetEnabled,
      BlueprintSetter = SetEnabled,
      meta = (AllowPrivateAccess = true))
  bool Enabled = true;

  /**
   * The latitude of the georeference origin for this sub-level in degrees, in
   * the range [-90, 90]. When this sub-level is active, the CesiumGeoreference
   * will adopt this origin.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      BlueprintGetter = GetOriginLatitude,
      BlueprintSetter = SetOriginLatitude,
      meta = (ClampMin = -90.0, ClampMax = 90.0, AllowPrivateAccess = true))
  double OriginLatitude = 39.736401;

  /**
   * The longitude of the georeference origin for this sub-level in degrees, in
   * the range [-180, 180]. When this sub-level is active, the
   * CesiumGeoreference will adopt this origin.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      BlueprintGetter = GetOriginLongitude,
      BlueprintSetter = SetOriginLongitude,
      meta = (ClampMin = -180.0, ClampMax = 180.0, AllowPrivateAccess = true))
  double OriginLongitude = -105.25737;

  /**
   * The height of the georeference origin for this sub-level in meters above
   * the ellipsoid. This height should not be confused with a height above
   * Mean Sea Level. When this sub-level is active, the CesiumGeoreference will
   * adopt this origin.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      BlueprintGetter = GetOriginHeight,
      BlueprintSetter = SetOriginHeight,
      meta = (AllowPrivateAccess = true))
  double OriginHeight = 2250.0;

  /**
   * How close to the sub-level local origin, in meters, the camera needs to be
   * to load the level.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      BlueprintGetter = GetLoadRadius,
      BlueprintSetter = SetLoadRadius,
      meta = (ClampMin = 0.0, AllowPrivateAccess = true))
  double LoadRadius = 1000.0;

  /**
   * The designated georeference actor controlling how the actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the sub-level will find and use the first Georeference
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
  TSoftObjectPtr<ACesiumGeoreference> Georeference;

  /**
   * The resolved georeference used by this sub-level. This is not serialized
   * because it may point to a Georeference in the PersistentLevel while this
   * Actor is in a sub-level. If the Georeference property is specified,
   * however then this property will have the same value.
   *
   * This property will be null before ResolveGeoreference is called.
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
   * Gets the sub-level switch component with which this sub-level is
   * associated. Calling this method will call ResolveGeoreference to resolve
   * the georeference, if it's not already resolved.
   */
  UCesiumSubLevelSwitcherComponent* _getSwitcher() noexcept;

  /**
   * Gets the Level Instance Actor to which this component is attached. If this
   * component is not attached to a Level Instance Actor, this method logs a
   * warning and returns nullptr.
   */
  ALevelInstance* _getLevelInstance() const noexcept;

  /**
   * Invalidates the cached resolved georeference, unsubscribing from it and
   * setting it to null. The next time ResolveGeoreference is called, the
   * Georeference will be re-resolved and re-subscribed.
   */
  void _invalidateResolvedGeoreference();

  void PlaceOriginAtEcef(const FVector& NewOriginEcef);
};
