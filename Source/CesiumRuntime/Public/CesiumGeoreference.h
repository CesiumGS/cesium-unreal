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

class ICesiumGeoreferenceListener;
class ICesiumBoundingVolumeProvider;

class APlayerCameraManager;

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
 * used by other Cesium Actors to control how this coordinate system is mapped
 * into an Unreal Engine world and level.
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
      Category = "CesiumSublevels",
      meta = (EditCondition = "KeepWorldOriginNearCamera"))
  bool OriginRebaseInsideSublevels = true;

  /*
   * Whether to visualize the level loading radii in the editor. Helpful for
   * initially positioning the level and choosing a load radius.
   */
  UPROPERTY(EditAnywhere, Category = "CesiumSublevels")
  bool ShowLoadRadii = true;

  /*
   * Rescan for sublevels that have not been georeferenced yet. New levels are
   * placed at the Unreal origin and georeferenced automatically.
   */
  UFUNCTION(CallInEditor, Category = "CesiumSublevels")
  void CheckForNewSubLevels();

  /*
   * Jump to the level specified by "Current Level Index".
   *
   * Warning: Before clicking, ensure that all non-Cesium objects in the
   * persistent level are georeferenced with the "CesiumGeoreferenceComponent"
   * or attached to a an actor with that component. Ensure that static actors
   * only exist in georeferenced sublevels.
   */
  UFUNCTION(CallInEditor, Category = "CesiumSublevels")
  void JumpToCurrentLevel();

  /*
   * The index of the level the georeference origin should be set to. This
   * aligns the globe with the specified level so that it can be worked on in
   * the editor.
   *
   * Warning: Before changing, ensure the last level you worked on has been
   * properly georeferenced. Ensure all actors are georeferenced, either by
   * inclusion in a georeferenced sublevel, by adding the
   * "CesiumGeoreferenceComponent", or by attaching to an actor with one.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "CesiumSublevels",
      meta = (ArrayClamp = "CesiumSubLevels"))
  int CurrentLevelIndex;

  /*
   * The list of georeferenced sublevels. Each of these has a corresponding
   * world location that can be jumped to. Only one level can be worked on in
   * the editor at a time.
   */
  UPROPERTY(EditAnywhere, EditFixedSize, Category = "CesiumSublevels")
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
   * The height of the custom origin placement in meters above the WGS84
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
  * Returns the georeference origin position as an FVector. Only valid if
  * the placement type is Cartographic Origin (i.e. Longitude / Latitude / Height).
  */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateGetGeoreferenceOriginLongitudeLatitudeHeight() const;
  
  /**
   * This aligns the specified WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) to Unreal's world origin. I.e. it
   * rotates the globe so that these coordinates exactly fall on the origin.
   */
  void SetGeoreferenceOrigin(const glm::dvec3& TargetLongitudeLatitudeHeight);

  /**
   * This aligns the specified WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) to Unreal's world origin. I.e. it
   * rotates the globe so that these coordinates exactly fall on the origin.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void
  InaccurateSetGeoreferenceOrigin(const FVector& TargetLongitudeLatitudeHeight);

  /*
   * USEFUL CONVERSION FUNCTIONS
   */

  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Earth-Centered, Earth-Fixed
   * (ECEF) coordinates.
   */
  glm::dvec3 TransformLongitudeLatitudeHeightToEcef(
      const glm::dvec3& LongitudeLatitudeHeight) const;

  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Earth-Centered, Earth-Fixed
   * (ECEF) coordinates.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformLongitudeLatitudeHeightToEcef(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  glm::dvec3
  TransformEcefToLongitudeLatitudeHeight(const glm::dvec3& Ecef) const;

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector
  InaccurateTransformEcefToLongitudeLatitudeHeight(const FVector& Ecef) const;

  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Unreal world coordinates
   * (relative to the floating origin).
   */
  glm::dvec3 TransformLongitudeLatitudeHeightToUnreal(
      const glm::dvec3& LongitudeLatitudeHeight) const;

  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Unreal world coordinates
   * (relative to the floating origin).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformLongitudeLatitudeHeightToUnreal(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  glm::dvec3
  TransformUnrealToLongitudeLatitudeHeight(const glm::dvec3& Ue) const;

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector
  InaccurateTransformUnrealToLongitudeLatitudeHeight(const FVector& Ue) const;

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal relative world (relative to the floating origin).
   */
  glm::dvec3 TransformEcefToUnreal(const glm::dvec3& Ecef) const;

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal relative world (relative to the floating origin).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformEcefToUnreal(const FVector& Ecef) const;

  /**
   * Transforms the given point from Unreal relative world (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   */
  glm::dvec3 TransformUnrealToEcef(const glm::dvec3& Ue) const;

  /**
   * Transforms the given point from Unreal relative world (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FVector InaccurateTransformUnrealToEcef(const FVector& Ue) const;

  /**
   * Transforms a rotator from Unreal world to East-North-Up at the given
   * Unreal relative world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorUnrealToEastNorthUp(
      const glm::dvec3& origin,
      const glm::dquat& UeRotator,
      const glm::dvec3& UeLocation) const;

  /**
   * Transforms a rotator from Unreal world to East-North-Up at the given
   * Unreal relative world location (relative to the floating origin).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FRotator InaccurateTransformRotatorUnrealToEastNorthUp(
      const FIntVector& Origin,
      const FRotator& UeRotator,
      const FVector& UeLocation) const;

  /**
   * Transforms a rotator from East-North-Up to Unreal world at the given
   * Unreal relative world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorEastNorthUpToUnreal(
      const glm::dvec3& origin,
      const glm::dquat& EnuRotator,
      const glm::dvec3& UeLocation) const;

  /**
   * Transforms a rotator from East-North-Up to Unreal world at the given
   * Unreal relative world location (relative to the floating origin).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FRotator InaccurateTransformRotatorEastNorthUpToUnreal(
      const FIntVector& Origin,
      const FRotator& EnuRotator,
      const FVector& UeLocation) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to Unreal at the
   * specified Unreal relative world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   */
  glm::dmat3 ComputeEastNorthUpToUnreal(const glm::dvec3& Ue) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to Unreal at the
   * specified Unreal relative world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FMatrix InaccurateComputeEastNorthUpToUnreal(const FVector& Ue) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  glm::dmat3 ComputeEastNorthUpToEcef(const glm::dvec3& Ecef) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FMatrix InaccurateComputeEastNorthUpToEcef(const FVector& Ecef) const;

  /*
   * GEOREFERENCE TRANSFORMS
   */

  /**
   * @brief Gets the transformation from the "Georeferenced" reference frame
   * defined by this instance to the "Ellipsoid-centered" reference frame (i.e.
   * ECEF).
   *
   * Gets a matrix that transforms coordinates from the "Georeference" reference
   * frame defined by this instance to the "Ellipsoid-centered" reference frame,
   * which is usually Earth-centered, Earth-fixed.
   * See {@link reference-frames.md}.
   */
  const glm::dmat4& GetGeoreferencedToEllipsoidCenteredTransform() const {
    return this->_geoTransforms.GetGeoreferencedToEllipsoidCenteredTransform();
  }

  /**
   * @brief Gets the transformation from the "Ellipsoid-centered" reference
   * frame (i.e. ECEF) to the georeferenced reference frame defined by this
   * instance.
   *
   * Gets a matrix that transforms coordinates from the "Ellipsoid-centered"
   * reference frame (which is usually Earth-centered, Earth-fixed) to the
   * "Georeferenced" reference frame defined by this instance.
   * See {@link reference-frames.md}.
   */
  const glm::dmat4& GetEllipsoidCenteredToGeoreferencedTransform() const {
    return this->_geoTransforms.GetEllipsoidCenteredToGeoreferencedTransform();
  }

  /**
   * @brief Gets the transformation from the "Unreal World" reference frame to
   * the "Ellipsoid-centered" reference frame (i.e. ECEF).
   *
   * Gets a matrix that transforms coordinates from the "Unreal World" reference
   * frame (with respect to the absolute world origin, not the floating origin)
   * to the "Ellipsoid-centered" reference frame (which is usually
   * Earth-centered, Earth-fixed). See {@link reference-frames.md}.
   */
  const glm::dmat4& GetUnrealWorldToEllipsoidCenteredTransform() const {
    return this->_geoTransforms.GetUnrealWorldToEllipsoidCenteredTransform();
  }

  /**
   * @brief Gets the transformation from the "Ellipsoid-centered" reference
   * frame (i.e. ECEF) to the "Unreal World" reference frame.
   *
   * Gets a matrix that transforms coordinates from the "Ellipsoid-centered"
   * reference frame (which is usually Earth-centered, Earth-fixed) to the
   * "Unreal world" reference frame (with respect to the absolute world origin,
   * not the floating origin). See {@link reference-frames.md}.
   */
  const glm::dmat4& GetEllipsoidCenteredToUnrealWorldTransform() const {
    return this->_geoTransforms.GetEllipsoidCenteredToUnrealWorldTransform();
  }

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
   * Adds a ICesiumGeoreferenceListener to be notified on changes to the world
   * georeference transforms.
   */
  void AddGeoreferenceListener(ICesiumGeoreferenceListener* Object);

  /**
   * Adds a ICesiumBoundingVolumeProvider that will contribute to the
   * georeference origin placement when OriginPlacement =
   * EOriginPlacement::BoundingVolumeOrigin. Other origin placement modes will
   * be unaffected by bounding volume providers.
   */
  void AddBoundingVolumeProvider(ICesiumBoundingVolumeProvider* Object);

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

  // TODO : Try to avoid exposing this publicly
  const GeoTransforms& getGeoTransforms() const noexcept {
    return _geoTransforms;
  }

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;
  virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

  /**
   * Called after the C++ constructor and after the properties have
   * been initialized, including those loaded from config.
   */
  void PostInitProperties() override;

private:
  /**
   * A tag that is assigned to Georeferences when they are created
   * as the "default" Georeference for a certain world.
   */
  static FName DEFAULT_GEOREFERENCE_TAG;

  UPROPERTY()
  double _ellipsoidRadii[3];

  UPROPERTY()
  double _center[3];

  GeoTransforms _geoTransforms;

  bool _insideSublevel;

  // TODO: add option to set georeference directly from ECEF
  void _setGeoreferenceOrigin(
      double targetLongitude,
      double targetLatitude,
      double targetHeight);
  void _jumpToLevel(const FCesiumSubLevel& level);

  /**
   * Will make sure that the `CesiumSubLevels` array contains entries
   * that exactly match the current streaming levels of the world
   */
  void _updateCesiumSubLevels();

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

  TArray<TWeakInterfacePtr<ICesiumGeoreferenceListener>> _georeferenceListeners;
  TArray<TWeakInterfacePtr<ICesiumBoundingVolumeProvider>>
      _boundingVolumeProviders;

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
};
