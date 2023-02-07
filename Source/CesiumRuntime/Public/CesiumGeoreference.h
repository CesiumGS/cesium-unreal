// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumSubLevel.h"
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeoTransforms.h"
#include "OriginPlacement.h"
#include "UObject/WeakInterfacePtr.h"
#include <glm/mat3x3.hpp>
#include "CesiumGeoreference.generated.h"

class APlayerCameraManager;
class FLevelCollectionModel;

/**
 * The delegate for the ACesiumGeoreference::OnGeoreferenceUpdated,
 * which is triggered from UpdateGeoreference
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoreferenceUpdated);

/**
 * Controls how global geospatial coordinates are mapped to coordinates in the
 * Unreal Engine level. Internally, Cesium uses a global Earth-centered,
 * Earth-fixed (ECEF) ellipsoid-centered coordinate system, where the ellipsoid
 * is usually the World Geodetic System 1984 (WGS84) ellipsoid. This is a
 * right-handed system centered at the Earth's center of mass, where +X is in
 * the direction of the intersection of the Equator and the Prime Meridian (zero
 * degrees longitude), +Y is in the direction of the intersection of the Equator
 * and +90 degrees longitude, and +Z is through the North Pole. This Actor is
 * used by other Cesium Actors and components to control how this coordinate
 * system is mapped into an Unreal Engine world and level.
 */
UCLASS()
class CESIUMRUNTIME_API ACesiumGeoreference : public AActor {
  GENERATED_BODY()

public:
  /*
   * Finds and returns the actor labeled `CesiumGeoreferenceDefault` in the
   * persistent level of the calling object's world. If not found, it creates a
   * new default Georeference.
   * @param WorldContextObject Any `UObject`.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "CesiumGeoreference",
      meta = (WorldContext = "WorldContextObject"))
  static ACesiumGeoreference*
  GetDefaultGeoreference(const UObject* WorldContextObject);

  ACesiumGeoreference();

  /*
   * Whether to continue origin rebasing once inside a sublevel. If actors
   * inside the sublevels react poorly to origin rebasing, it might be worth
   * turning this option off.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Cesium Sublevels",
      meta = (EditCondition = "KeepWorldOriginNearCamera"))
  bool OriginRebaseInsideSublevels = true;

  /*
   * Whether to visualize the level loading radii in the editor. Helpful for
   * initially positioning the level and choosing a load radius.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Cesium Sublevels")
  bool ShowLoadRadii = true;

  /*
   * Switches to the specified level. Sets the georeference origin to the given
   * level's origin, shows the given level, and hides all other levels.
   *
   * If Index is negative or otherwise outside the range of valid indices, all
   * levels other than the PersistentLevel are deactivated.
   *
   * This function is meant to be called from within the game, not in the
   * Editor.
   *
   * @returns true if a new sub-level is active, or false if the Index was
   * outside the valid range and so no sub-level is active.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Cesium Sublevels")
  bool SwitchToLevel(int32 Index);

  /*
   * The list of georeferenced sublevels. Each of these has a corresponding
   * world location that can be jumped to. Only one level can be worked on in
   * the editor at a time.
   *
   * New levels added in the Editor should appear in this list automatically. If
   * any are missing, check that "World Composition" is enabled in World
   * Settings and that the level is in a Layer with Distance Based Streaming
   * DISABLED.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Cesium Sublevels",
      Meta =
          (TitleProperty = "LevelName",
           DisplayName = "Georeferenced Sublevels"))
  TArray<FCesiumSubLevel> CesiumSubLevels;

  /**
   * The placement of this Actor's origin (coordinate 0,0,0) within the tileset.
   *
   * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates, such
   * that the tileset content is in a small bounding volume 6-7 million meters
   * (the radius of the Earth) away from the coordinate system origin. This
   * property allows an alternative position, other then the tileset's true
   * origin, to be treated as the origin for the purpose of this Actor. Using
   * this property will preserve vertex precision (and thus avoid jittering)
   * much better than setting the Actor's Transform property.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cesium")
  EOriginPlacement OriginPlacement = EOriginPlacement::CartographicOrigin;

  /**
   * The latitude of the custom origin placement in degrees, in the range [-90,
   * 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta =
          (EditCondition =
               "OriginPlacement==EOriginPlacement::CartographicOrigin",
           ClampMin = -90.0,
           ClampMax = 90.0))
  double OriginLatitude = 39.736401;

  /**
   * The longitude of the custom origin placement in degrees, in the range
   * [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta =
          (EditCondition =
               "OriginPlacement==EOriginPlacement::CartographicOrigin",
           ClampMin = -180.0,
           ClampMax = 180.0))
  double OriginLongitude = -105.25737;

  /**
   * The height of the custom origin placement in meters above the
   * ellipsoid.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta =
          (EditCondition =
               "OriginPlacement==EOriginPlacement::CartographicOrigin"))
  double OriginHeight = 2250.0;

  /**
   * TODO: Once point-and-click georeference placement is in place, restore this
   * as a UPROPERTY
   */
  // UPROPERTY(EditAnywhere, Category = "Cesium", AdvancedDisplay)
  bool EditOriginInViewport = false;

  /**
   * If true, the world origin is periodically rebased to keep it near the
   * camera.
   *
   * This is important for maintaining vertex precision in large worlds. Setting
   * it to false can lead to jiterring artifacts when the camera gets far away
   * from the origin.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool KeepWorldOriginNearCamera = true;

#if WITH_EDITOR
  /**
   * Places the georeference origin at the camera's current location. Rotates
   * the globe so the current longitude/latitude/height of the camera is at the
   * Unreal origin. The camera is also teleported to the Unreal origin.
   *
   * Warning: Before clicking, ensure that all non-Cesium objects in the
   * persistent level are georeferenced with the "CesiumGeoreferenceComponent"
   * or attached to an actor with that component. Ensure that static actors only
   * exist in georeferenced sublevels.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void PlaceGeoreferenceOriginHere();
#endif

  /**
   * The maximum distance in centimeters that the camera may move from the
   * world's OriginLocation before the world origin is moved closer to the
   * camera.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (EditCondition = "KeepWorldOriginNearCamera", ClampMin = 0.0))
  double MaximumWorldOriginDistanceFromCamera = 10000.0;

  /**
   * The camera to use for setting the world origin.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (EditCondition = "KeepWorldOriginNearCamera"))
  APlayerCameraManager* WorldOriginCamera;

  // TODO: Allow user to select/configure the ellipsoid.
  // Yeah, we're working on that...

  /**
   * Returns the georeference origin position as an FVector where `X` is
   * longitude (degrees), `Y` is latitude (degrees), and `Z` is height above the
   * ellipsoid (meters). Only valid if the placement type is Cartographic Origin
   * (i.e. Longitude / Latitude / Height).
   *
   * This converts the values to single-precision floating point values.
   * The double-precision values can be accessed via the
   * OriginLongitude, OriginLatitude and OriginHeight properties.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateGetGeoreferenceOriginLongitudeLatitudeHeight() const;

  /**
   * This aligns the specified longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) to Unreal's world
   * origin. I.e. it moves the globe so that these coordinates exactly fall on
   * the origin.
   *
   * When the WorldOriginCamera of this instance is currently contained
   * the bounds of a sublevel, then this call has no effect.
   */
  void SetGeoreferenceOriginLongitudeLatitudeHeight(
      const glm::dvec3& TargetLongitudeLatitudeHeight);

  /**
   * This function is here for backwards compatibility. The function
   * SetGeoreferenceOriginLongitudeLatitudeHeight should be used instead.
   */
  void SetGeoreferenceOrigin(const glm::dvec3& TargetLongitudeLatitudeHeight);

  /**
   * This aligns the specified Earth-Centered, Earth-Fixed (ECEF) coordinates to
   * Unreal's world origin. I.e. it moves the globe so that these coordinates
   * exactly fall on the origin.
   *
   * When the WorldOriginCamera of this instance is currently contained
   * the bounds of a sublevel, then this call has no effect.
   */
  void SetGeoreferenceOriginEcef(const glm::dvec3& TargetEcef);

  /**
   * This aligns the specified longitude in degrees (X), latitude in
   * degrees (Y), and height above the ellipsoid in meters (Z) to Unreal's world
   * origin. I.e. it moves the globe so that these coordinates exactly fall on
   * the origin.
   *
   * When the WorldOriginCamera of this instance is currently contained
   * the bounds of a sublevel, then this call has no effect.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateSetGeoreferenceOriginLongitudeLatitudeHeight(
      const FVector& TargetLongitudeLatitudeHeight);

  /**
   * This aligns the specified Earth-Centered, Earth-Fixed (ECEF) coordinates to
   * Unreal's world origin. I.e. it moves the globe so that these coordinates
   * exactly fall on the origin.
   *
   * When the WorldOriginCamera of this instance is currently contained
   * the bounds of a sublevel, then this call has no effect.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InaccurateSetGeoreferenceOriginEcef(const FVector& TargetEcef);

  /*
   * USEFUL CONVERSION FUNCTIONS
   */

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Earth-Centered, Earth-Fixed
   * (ECEF) coordinates.
   */
  glm::dvec3 TransformLongitudeLatitudeHeightToEcef(
      const glm::dvec3& LongitudeLatitudeHeight) const;

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) into
   * Earth-Centered, Earth-Fixed (ECEF) coordinates.
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformLongitudeLatitudeHeightToEcef can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformLongitudeLatitudeHeightToEcef(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height above
   * the ellipsoid in meters (z).
   */
  glm::dvec3
  TransformEcefToLongitudeLatitudeHeight(const glm::dvec3& Ecef) const;

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height above
   * the ellipsoid in meters (z).
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformEcefToLongitudeLatitudeHeight can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector
  InaccurateTransformEcefToLongitudeLatitudeHeight(const FVector& Ecef) const;

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) into Unreal world
   * coordinates (relative to the floating origin).
   */
  glm::dvec3 TransformLongitudeLatitudeHeightToUnreal(
      const glm::dvec3& longitudeLatitudeHeight) const;

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) into Unreal world
   * coordinates (relative to the floating origin).
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformLongitudeLatitudeHeightToUnreal can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformLongitudeLatitudeHeightToUnreal(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * longitude in degrees (x), latitude in degrees (y), and height above the
   * ellipsoid in meters (z).
   */
  glm::dvec3
  TransformUnrealToLongitudeLatitudeHeight(const glm::dvec3& unreal) const;

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * longitude in degrees (x), latitude in degrees (y), and height above the
   * ellipsoid in meters (z).
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformUnrealToLongitudeLatitudeHeight can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformUnrealToLongitudeLatitudeHeight(
      const FVector& Unreal) const;

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal relative world (relative to the floating origin).
   */
  glm::dvec3 TransformEcefToUnreal(const glm::dvec3& ecef) const;

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal relative world (relative to the floating origin).
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformEcefToUnreal can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformEcefToUnreal(const FVector& Ecef) const;

  /**
   * Transforms the given point from Unreal relative world (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   */
  glm::dvec3 TransformUnrealToEcef(const glm::dvec3& unreal) const;

  /**
   * Transforms the given point from Unreal relative world (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformUnrealToEcef can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformUnrealToEcef(const FVector& Unreal) const;

  /**
   * Transforms a rotator from Unreal world to East-South-Up at the given
   * Unreal world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorUnrealToEastSouthUp(
      const glm::dquat& UnrealRotator,
      const glm::dvec3& UnrealLocation) const;

  /**
   * Transforms a rotator from Unreal world to East-South-Up at the given
   * Unreal world location (relative to the floating origin).
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformRotatorUnrealToEastSouthUp can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FRotator InaccurateTransformRotatorUnrealToEastSouthUp(
      const FRotator& UnrealRotator,
      const FVector& UnrealLocation) const;

  /**
   * Transforms a rotator from East-South-Up to Unreal world at the given
   * Unreal world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorEastSouthUpToUnreal(
      const glm::dquat& EsuRotator,
      const glm::dvec3& UnrealLocation) const;

  /**
   * Transforms a rotator from East-South-Up to Unreal world at the given
   * Unreal world location (relative to the floating origin).
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformRotatorEastSouthUpToUnreal can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FRotator InaccurateTransformRotatorEastSouthUpToUnreal(
      const FRotator& EsuRotator,
      const FVector& UnrealLocation) const;

  /**
   * Computes the rotation matrix from the local East-South-Up to Unreal at the
   * specified Unreal world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   */
  glm::dmat3 ComputeEastSouthUpToUnreal(const glm::dvec3& unreal) const;

  /**
   * Computes the rotation matrix from the local East-South-Up to Unreal at the
   * specified Unreal world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * ComputeEastSouthUpToUnreal can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FMatrix InaccurateComputeEastSouthUpToUnreal(const FVector& Unreal) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  glm::dmat3 ComputeEastNorthUpToEcef(const glm::dvec3& ecef) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * ComputeEastNorthUpToEcef can be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FMatrix InaccurateComputeEastNorthUpToEcef(const FVector& Ecef) const;

  /**
   * @brief Computes the normal of the plane tangent to the surface of the
   * ellipsoid that is used by this instance, at the provided position.
   *
   * @param position The cartesian position for which to to determine the
   * surface normal.
   * @return The normal.
   */
  glm::dvec3 ComputeGeodeticSurfaceNormal(const glm::dvec3& position) const {
    return _geoTransforms.ComputeGeodeticSurfaceNormal(position);
  }
  /**
   * A delegate that will be called whenever the Georeference is
   * modified in a way that affects its computations.
   */
  UPROPERTY(BlueprintAssignable, Category = "Cesium")
  FGeoreferenceUpdated OnGeoreferenceUpdated;

  /**
   * Recomputes all world georeference transforms. Usually there is no need to
   * explicitly call this from external code.
   */
  void UpdateGeoreference();

  /**
   * @brief Returns whether `Tick` should be called in viewports-only mode.
   *
   * "If `true`, actor is ticked even if TickType==LEVELTICK_ViewportsOnly."
   * (The TickType is determined by the unreal engine internally).
   */
  virtual bool ShouldTickIfViewportsOnly() const override;

  /**
   * @brief Function called every frame on this Actor.
   *
   * @param DeltaTime Game time elapsed during last frame modified by the time
   * dilation
   */
  virtual void Tick(float DeltaTime) override;

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
   * Returns the GeoTransforms that offers the same conversion
   * functions as this class, but performs the computations
   * in double precision.
   */
  const GeoTransforms& GetGeoTransforms() const noexcept {
    return _geoTransforms;
  }

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void OnConstruction(const FTransform& Transform) override;
  virtual void BeginDestroy() override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
  /**
   * A tag that is assigned to Georeferences when they are created
   * as the "default" Georeference for a certain world.
   */
  static FName DEFAULT_GEOREFERENCE_TAG;

  /**
   * The radii, in x-, y-, and z-direction, of the ellipsoid that
   * should be used in this instance.
   */
  UPROPERTY()
  double _ellipsoidRadii[3];

  GeoTransforms _geoTransforms;

  bool _insideSublevel;

#if WITH_EDITOR
  FDelegateHandle _newCurrentLevelSubscription;
#endif

  // TODO: add option to set georeference directly from ECEF
  void _setGeoreferenceOrigin(
      double targetLongitude,
      double targetLatitude,
      double targetHeight);
  void _jumpToLevel(const FCesiumSubLevel& level);

#if WITH_EDITOR
  /**
   * Will make sure that the `CesiumSubLevels` array contains all
   * of the current streaming levels of the world.
   */
  void _updateCesiumSubLevels();
#endif

#if WITH_EDITOR
  void _lineTraceViewportMouse(
      const bool ShowTrace,
      bool& Success,
      FHitResult& HitResult) const;

  /**
   * @brief Show the load radius of each sub-level as a sphere.
   *
   * If this is not called "in-game", and `ShowLoadRadii` is `true`,
   * then it will show a sphere indicating the load radius of each
   * sub-level.
   */
  void _showSubLevelLoadRadii() const;

  /**
   * @brief Allow editing the origin with the mouse.
   *
   * If `EditOriginInViewport` is true, this will trace the mouse
   * position, and update the origin based on the point that was
   * hit.
   */
  void _handleViewportOriginEditing();

#endif

  /**
   * @brief Updates the load state of sublevels.
   *
   * This checks all sublevels whether their load radius contains the
   * `WorldOriginCamera`, in ECEF coordinates. The sublevels that
   * contain the camera will be loaded. All others will be unloaded.
   *
   * @return Whether the camera is contained in *any* sublevel.
   */
  bool _updateSublevelState();

  /**
   * @brief Perform the origin-rebasing.
   *
   * If this actor is currently "in-game", and has an associated
   * `WorldOriginCamera`, and the camera is further away from the origin than
   * `MaximumWorldOriginDistanceFromCamera`, then this may set a new world
   * origin by calling `GetWorld()->SetNewWorldOrigin` with a new position.
   *
   * This will only be done if origin rebasing is enabled via
   * `KeepWorldOriginNearCamera`, and the actor is either *not* in a sublevel,
   * or `OriginRebaseInsideSublevels` is enabled.
   */
  void _performOriginRebasing();

  /**
   * Updates _geoTransforms based on the current ellipsoid and center, and
   * returns the old transforms.
   */
  void _updateGeoTransforms();

  /**
   * @brief Finds the ULevelStreaming with given name.
   *
   * @returns The ULevelStreaming, or nullptr if the provided name does not
   * exist.
   */
  ULevelStreaming* _findLevelStreamingByName(const FString& name);

  FCesiumSubLevel* _findCesiumSubLevelByName(
      const FName& packageName,
      bool createIfDoesNotExist);

#if WITH_EDITOR
  void _onNewCurrentLevel();
  void _enableAndGeoreferenceCurrentSubLevel();
#endif

  /**
   * Determines if this Georeference should manage sub-level switching.
   *
   * A Georeference inside a sub-level should not manage sub-level switching,
   * so this function returns true the Georeference is in the world's
   * PersistentLevel.
   */
  bool _shouldManageSubLevels() const;
};
