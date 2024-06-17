// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEllipsoid.h"
#include "CesiumGeospatial/LocalHorizontalCoordinateSystem.h"
#include "CesiumSubLevel.h"
#include "GameFramework/Actor.h"
#include "Delegates/Delegate.h"
#include "GeoTransforms.h"
#include "OriginPlacement.h"
#include "CesiumGeoreference.generated.h"

class APlayerCameraManager;
class FLevelCollectionModel;
class UCesiumSubLevelSwitcherComponent;

/**
 * The delegate for the ACesiumGeoreference::OnGeoreferenceUpdated,
 * which is triggered from UpdateGeoreference
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoreferenceUpdated);

/**
 * The event that triggers when a georeference's ellipsoid is changed.
 * This should be used for performing any necessary coordinate changes.
 * The parameters are (OldEllipsoid, NewEllipsoid).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FGeoreferenceEllipsoidChanged,
    UCesiumEllipsoid*,
    OldEllipsoid,
    UCesiumEllipsoid*,
    NewEllipsoid);

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
  /**
   * The minimum allowed value for the Scale property, 1e-6.
   */
  static const double kMinimumScale;

  /**
   * Finds and returns a CesiumGeoreference in the world. It searches in the
   * following order:
   *
   * 1. A CesiumGeoreference that is tagged with "DEFAULT_GEOREFERENCE" and
   * found in the PersistentLevel.
   * 2. A CesiumGeoreference with the name "CesiumGeoreferenceDefault" and found
   * in the PersistentLevel.
   * 3. Any CesiumGeoreference in the PersistentLevel.
   *
   * If no CesiumGeoreference is found with this search, a new one is created in
   * the persistent level and given the "DEFAULT_GEOREFERENCE" tag.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (WorldContext = "WorldContextObject"))
  static ACesiumGeoreference*
  GetDefaultGeoreference(const UObject* WorldContextObject);

  /**
   * Finds and returns the CesiumGeoreference suitable for use with the given
   * Actor. It searches in the following order:
   *
   * 1. A CesiumGeoreference that is an attachment parent of the given Actor.
   * 2. A CesiumGeoreference that is tagged with "DEFAULT_GEOREFERENCE" and
   * found in the PersistentLevel.
   * 3. A CesiumGeoreference with the name "CesiumGeoreferenceDefault" and found
   * in the PersistentLevel.
   * 4. Any CesiumGeoreference in the PersistentLevel.
   *
   * If no CesiumGeoreference is found with this search, a new one is created in
   * the persistent level and given the "DEFAULT_GEOREFERENCE" tag.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  static ACesiumGeoreference* GetDefaultGeoreferenceForActor(AActor* Actor);

  /**
   * A delegate that will be called whenever the Georeference is
   * modified in a way that affects its computations.
   */
  UPROPERTY(BlueprintAssignable, Category = "Cesium")
  FGeoreferenceUpdated OnGeoreferenceUpdated;

  /**
   * An event that will be called whenever the georeference's ellipsoid has
   * been modified.
   */
  UPROPERTY(BlueprintAssignable, Category = "Cesium")
  FGeoreferenceEllipsoidChanged OnEllipsoidChanged;

#pragma region Properties

private:
  /**
   * The Ellipsoid being used by this georeference. The ellipsoid informs how
   * cartographic coordinates will be interpreted and how they are transformed
   * into cartesian coordinates.
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetEllipsoid,
      BlueprintSetter = SetEllipsoid,
      meta = (AllowPrivateAccess))
  UCesiumEllipsoid* Ellipsoid;

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
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetOriginPlacement,
      BlueprintSetter = SetOriginPlacement,
      meta = (AllowPrivateAccess))
  EOriginPlacement OriginPlacement = EOriginPlacement::CartographicOrigin;

  /**
   * The latitude of the custom origin placement in degrees, in the range [-90,
   * 90]
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetOriginLatitude,
      BlueprintSetter = SetOriginLatitude,
      Interp,
      meta =
          (AllowPrivateAccess,
           EditCondition =
               "OriginPlacement==EOriginPlacement::CartographicOrigin",
           ClampMin = -90.0,
           ClampMax = 90.0))
  double OriginLatitude = 39.736401;

  /**
   * The longitude of the custom origin placement in degrees, in the range
   * [-180, 180]
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetOriginLongitude,
      BlueprintSetter = SetOriginLongitude,
      Interp,
      meta =
          (AllowPrivateAccess,
           EditCondition =
               "OriginPlacement==EOriginPlacement::CartographicOrigin",
           ClampMin = -180.0,
           ClampMax = 180.0))
  double OriginLongitude = -105.25737;

  /**
   * The height of the custom origin placement in meters above the
   * ellipsoid.
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetOriginHeight,
      BlueprintSetter = SetOriginHeight,
      Interp,
      meta =
          (AllowPrivateAccess,
           EditCondition =
               "OriginPlacement==EOriginPlacement::CartographicOrigin"))
  double OriginHeight = 2250.0;

  /**
   * The percentage scale of the globe in the Unreal world. If this value is 50,
   * for example, one meter on the globe occupies half a meter in the Unreal
   * world.
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintSetter = SetScale,
      BlueprintGetter = GetScale,
      Interp,
      Meta = (AllowPrivateAccess, UIMin = 0.000001, UIMax = 100.0))
  double Scale = 100.0;

  /**
   * The camera to use to determine which sub-level is closest, so that one can
   * be activated and all others deactivated.
   * @deprecated Add a CesiumOriginShiftComponent to the appropriate Actor
   * instead.
   */
  UPROPERTY(
      meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Add a CesiumOriginShiftComponent to the appropriate Actor instead."))
  APlayerCameraManager* SubLevelCamera_DEPRECATED = nullptr;

  /**
   * The component that allows switching between the sub-levels registered with
   * this georeference.
   */
  UPROPERTY(
      Instanced,
      Category = "Cesium|Sub-levels",
      BlueprintReadOnly,
      BlueprintGetter = GetSubLevelSwitcher,
      meta = (AllowPrivateAccess))
  UCesiumSubLevelSwitcherComponent* SubLevelSwitcher;

#if WITH_EDITORONLY_DATA
  /**
   * Whether to visualize the level loading radii in the editor. Helpful for
   * initially positioning the level and choosing a load radius.
   */
  UPROPERTY(
      Category = "Cesium|Sub-levels",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetShowLoadRadii,
      BlueprintSetter = SetShowLoadRadii,
      meta = (AllowPrivateAccess))
  bool ShowLoadRadii = true;
#endif

#pragma endregion

#pragma region PropertyAccessors

public:
  /**
   * Returns the georeference origin position as an FVector where `X` is
   * longitude (degrees), `Y` is latitude (degrees), and `Z` is height above the
   * ellipsoid (meters). Only valid if the placement type is Cartographic Origin
   * (i.e. Longitude / Latitude / Height).
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector GetOriginLongitudeLatitudeHeight() const;

  /**
   * This aligns the specified longitude in degrees (X), latitude in
   * degrees (Y), and height above the ellipsoid in meters (Z) to the Unreal
   * origin. That is, it moves the globe so that these coordinates exactly fall
   * on the origin. Only valid if the placement type is Cartographic Origin
   * (i.e. Longitude / Latitude / Height).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetOriginLongitudeLatitudeHeight(
      const FVector& TargetLongitudeLatitudeHeight);

  /**
   * Returns the georeference origin position as an FVector in Earth-Centerd,
   * Earth-Fixed (ECEF) coordinates. Only valid if the placement type is
   * Cartographic Origin (i.e. Longitude / Latitude / Height).
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector GetOriginEarthCenteredEarthFixed() const;

  /**
   * This aligns the specified Earth-Centered, Earth-Fixed (ECEF) coordinates to
   * the Unreal origin. That is, it moves the globe so that these coordinates
   * exactly fall on the origin. Only valid if the placement type is
   * Cartographic Origin (i.e. Longitude / Latitude / Height). Note that if the
   * provided ECEF coordiantes are near the center of the Earth so that
   * Longitude, Latitude, and Height are undefined, this function will instead
   * place the origin at 0 degrees longitude, 0 degrees latitude, and 0 meters
   * height about the ellipsoid.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetOriginEarthCenteredEarthFixed(
      const FVector& TargetEarthCenteredEarthFixed);

  /**
   * Gets the placement of this Actor's origin (coordinate 0,0,0) within the
   * tileset.
   *
   * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates, such
   * that the tileset content is in a small bounding volume 6-7 million meters
   * (the radius of the Earth) away from the coordinate system origin. This
   * property allows an alternative position, other then the tileset's true
   * origin, to be treated as the origin for the purpose of this Actor. Using
   * this property will preserve vertex precision (and thus avoid jittering)
   * much better than setting the Actor's Transform property.
   */
  UFUNCTION(BlueprintGetter)
  EOriginPlacement GetOriginPlacement() const;

  /**
   * Sets the placement of this Actor's origin (coordinate 0,0,0) within the
   * tileset.
   *
   * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates, such
   * that the tileset content is in a small bounding volume 6-7 million meters
   * (the radius of the Earth) away from the coordinate system origin. This
   * property allows an alternative position, other then the tileset's true
   * origin, to be treated as the origin for the purpose of this Actor. Using
   * this property will preserve vertex precision (and thus avoid jittering)
   * much better than setting the Actor's Transform property.
   */
  UFUNCTION(BlueprintSetter)
  void SetOriginPlacement(EOriginPlacement NewValue);

  /**
   * Gets the latitude of the custom origin placement in degrees, in the range
   * [-90, 90]
   */
  UFUNCTION(BlueprintGetter)
  double GetOriginLatitude() const;

  /**
   * Sets the latitude of the custom origin placement in degrees, in the range
   * [-90, 90]
   */
  UFUNCTION(BlueprintSetter)
  void SetOriginLatitude(double NewValue);

  /**
   * Gets the longitude of the custom origin placement in degrees, in the range
   * [-180, 180]
   */
  UFUNCTION(BlueprintGetter)
  double GetOriginLongitude() const;

  /**
   * Sets the longitude of the custom origin placement in degrees, in the range
   * [-180, 180]
   */
  UFUNCTION(BlueprintSetter)
  void SetOriginLongitude(double NewValue);

  /**
   * Gets the height of the custom origin placement in meters above the
   * ellipsoid.
   */
  UFUNCTION(BlueprintGetter)
  double GetOriginHeight() const;

  /**
   * Sets the height of the custom origin placement in meters above the
   * ellipsoid.
   */
  UFUNCTION(BlueprintSetter)
  void SetOriginHeight(double NewValue);

  /**
   * Gets the percentage scale of the globe in the Unreal world. If this value
   * is 50, for example, one meter on the globe occupies half a meter in the
   * Unreal world.
   */
  UFUNCTION(BlueprintGetter)
  double GetScale() const;

  /**
   * Sets the percentage scale of the globe in the Unreal world. If this value
   * is 50, for example, one meter on the globe occupies half a meter in the
   * Unreal world.
   */
  UFUNCTION(BlueprintSetter)
  void SetScale(double NewValue);

  /**
   * Gets the camera to use to determine which sub-level is closest, so that one
   * can be activated and all others deactivated.
   */
  UE_DEPRECATED(
      "Cesium For Unreal v2.0",
      "Add a CesiumOriginShiftComponent to the appropriate Actor instead.")
  UFUNCTION(BlueprintGetter)
  APlayerCameraManager* GetSubLevelCamera() const;

  /**
   * Sets the camera to use to determine which sub-level is closest, so that one
   * can be activated and all others deactivated.
   */
  UE_DEPRECATED(
      "Cesium For Unreal v2.0",
      "Add a CesiumOriginShiftComponent to the appropriate Actor instead.")
  UFUNCTION(BlueprintSetter)
  void SetSubLevelCamera(APlayerCameraManager* NewValue);

  /**
   * Gets the component that allows switching between different sub-levels
   * registered with this georeference.
   */
  UFUNCTION(BlueprintGetter)
  UCesiumSubLevelSwitcherComponent* GetSubLevelSwitcher() const {
    return this->SubLevelSwitcher;
  }

  /**
   * Returns a pointer to the UCesiumEllipsoid currently being used by this
   * georeference.
   */
  UFUNCTION(BlueprintCallable, BlueprintGetter, Category = "Cesium")
  UCesiumEllipsoid* GetEllipsoid() const;

  /**
   * Sets the UCesiumEllipsoid used by this georeference.
   *
   * Calling this will cause all tilesets under this georeference to be
   * reloaded.
   */
  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetEllipsoid(UCesiumEllipsoid* NewEllipsoid);

#if WITH_EDITOR
  /**
   * Gets whether to visualize the level loading radii in the editor. Helpful
   * for initially positioning the level and choosing a load radius.
   */
  UFUNCTION(BlueprintGetter)
  bool GetShowLoadRadii() const;

  /**
   * Sets whether to visualize the level loading radii in the editor. Helpful
   * for initially positioning the level and choosing a load radius.
   */
  UFUNCTION(BlueprintSetter)
  void SetShowLoadRadii(bool NewValue);
#endif // WITH_EDITOR

#pragma endregion

#pragma region Transformation Functions

public:
  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) into Unreal
   * coordinates. The resulting position should generally not be interpreted as
   * an Unreal _world_ position, but rather a position expressed in some parent
   * Actor's reference frame as defined by its transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "UnrealPosition"))
  FVector TransformLongitudeLatitudeHeightPositionToUnreal(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms a position in Unreal coordinates into longitude in degrees (x),
   * latitude in degrees (y), and height above the ellipsoid in meters (z). The
   * position should generally not be an Unreal _world_ position, but rather a
   * position expressed in some parent Actor's reference frame as defined by its
   * transform. This way, the chain of Unreal transforms places and orients the
   * "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "LongitudeLatitudeHeight"))
  FVector TransformUnrealPositionToLongitudeLatitudeHeight(
      const FVector& UnrealPosition) const;

  /**
   * Transforms a position in Earth-Centered, Earth-Fixed (ECEF) coordinates
   * into Unreal coordinates. The resulting position should generally not be
   * interpreted as an Unreal _world_ position, but rather a position expressed
   * in some parent Actor's reference frame as defined by its transform. This
   * way, the chain of Unreal transforms places and orients the "globe" in the
   * Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "UnrealPosition"))
  FVector TransformEarthCenteredEarthFixedPositionToUnreal(
      const FVector& EarthCenteredEarthFixedPosition) const;

  /**
   * Transforms the given position from Unreal coordinates to Earth-Centered,
   * Earth-Fixed (ECEF). The position should generally not be an Unreal _world_
   * position, but rather a position expressed in some parent Actor's reference
   * frame as defined by its transform. This way, the chain of Unreal transforms
   * places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EarthCenteredEarthFixedPosition"))
  FVector TransformUnrealPositionToEarthCenteredEarthFixed(
      const FVector& UnrealPosition) const;

  /**
   * Transforms a direction vector in Earth-Centered, Earth-Fixed (ECEF)
   * coordinates into Unreal coordinates. The resulting direction vector should
   * generally not be interpreted as an Unreal _world_ vector, but rather a
   * vector expressed in some parent Actor's reference frame as defined by its
   * transform. This way, the chain of Unreal transforms places and orients the
   * "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "UnrealDirection"))
  FVector TransformEarthCenteredEarthFixedDirectionToUnreal(
      const FVector& EarthCenteredEarthFixedDirection) const;

  /**
   * Transforms the given direction vector from Unreal coordinates to
   * Earth-Centered, Earth-Fixed (ECEF) coordinates. The direction vector should
   * generally not be an Unreal _world_ vector, but rather a vector expressed in
   * some parent Actor's reference frame as defined by its transform. This way,
   * the chain of Unreal transforms places and orients the "globe" in the Unreal
   * world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EarthCenteredEarthFixedPosition"))
  FVector TransformUnrealDirectionToEarthCenteredEarthFixed(
      const FVector& UnrealDirection) const;

  /**
   * Given a Rotator that transforms an object into the Unreal coordinate
   * system, returns a new Rotator that transforms that object into an
   * East-South-Up frame centered at a given location.
   *
   * In an East-South-Up frame, +X points East, +Y points South, and +Z points
   * Up. However, the directions of "East", "South", and "Up" in Unreal or ECEF
   * coordinates vary depending on where on the globe we are talking about.
   * That is why this function takes a location, expressed in Unreal
   * coordinates, that defines the origin of the East-South-Up frame of
   * interest.
   *
   * The Unreal location and the resulting Rotator should generally not be
   * relative to the Unreal _world_, but rather be expressed in some parent
   * Actor's reference frame as defined by its Transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EastSouthUpRotator"))
  FRotator TransformUnrealRotatorToEastSouthUp(
      const FRotator& UnrealRotator,
      const FVector& UnrealLocation) const;

  /**
   * Given a Rotator that transforms an object into the East-South-Up frame
   * centered at a given location, returns a new Rotator that transforms that
   * object into Unreal coordinates.
   *
   * In an East-South-Up frame, +X points East, +Y points South, and +Z points
   * Up. However, the directions of "East", "South", and "Up" in Unreal or ECEF
   * coordinates vary depending on where on the globe we are talking about.
   * That is why this function takes a location, expressed in Unreal
   * coordinates, that defines the origin of the East-South-Up frame of
   * interest.
   *
   * The Unreal location and the resulting Rotator should generally not be
   * relative to the Unreal _world_, but rather be expressed in some parent
   * Actor's reference frame as defined by its Transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "UnrealRotator"))
  FRotator TransformEastSouthUpRotatorToUnreal(
      const FRotator& EastSouthUpRotator,
      const FVector& UnrealLocation) const;

  /**
   * Computes the transformation matrix from the Unreal coordinate system to the
   * Earth-Centered, Earth-Fixed (ECEF) coordinate system. The Unreal
   * coordinates should generally not be interpreted as Unreal _world_
   * coordinates, but rather a coordinate system based on some parent Actor's
   * reference frame as defined by its transform. This way, the chain of Unreal
   * transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "UnrealToEarthCenteredEarthFixedMatrix"))
  FMatrix ComputeUnrealToEarthCenteredEarthFixedTransformation() const;

  /**
   * Computes the transformation matrix from the Earth-Centered, Earth-Fixed
   * (ECEF) coordinate system to the Unreal coordinate system. The Unreal
   * coordinates should generally not be interpreted as Unreal _world_
   * coordinates, but rather a coordinate system based on some parent Actor's
   * reference frame as defined by its transform. This way, the chain of Unreal
   * transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EarthCenteredEarthFixedToUnrealMatrix"))
  FMatrix ComputeEarthCenteredEarthFixedToUnrealTransformation() const;

  /**
   * Computes the matrix that transforms from an East-South-Up frame centered at
   * a given location to the Unreal frame.
   *
   * In an East-South-Up frame, +X points East, +Y points South, and +Z points
   * Up. However, the directions of "East", "South", and "Up" in Unreal or ECEF
   * coordinates vary depending on where on the globe we are talking about.
   * That is why this function takes a location, expressed in Unreal
   * coordinates, that defines the origin of the East-South-Up frame of
   * interest.
   *
   * The Unreal location and the resulting matrix should generally not be
   * relative to the Unreal _world_, but rather be expressed in some parent
   * Actor's reference frame as defined by its Transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EastSouthUpToUnrealMatrix"))
  FMatrix
  ComputeEastSouthUpToUnrealTransformation(const FVector& UnrealLocation) const;

  /**
   * Computes the matrix that transforms from an East-South-Up frame centered at
   * a given location to the Unreal frame. The location is expressed as an
   * Earth-Centered, Earth-Fixed (ECEF) position. To use an Unreal position
   * instead, use ComputeUnrealToEastSouthUpTransformation.
   *
   * In an East-South-Up frame, +X points East, +Y points South, and +Z points
   * Up. However, the directions of "East", "South", and "Up" in Unreal or ECEF
   * coordinates vary depending on where on the globe we are talking about.
   * That is why this function takes a location, expressed in ECEF
   * coordinates, that defines the origin of the East-South-Up frame of
   * interest.
   *
   * The resulting matrix should generally not be relative to the Unreal
   * _world_, but rather be expressed in some parent Actor's reference frame as
   * defined by its Transform. This way, the chain of Unreal transforms places
   * and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "EastSouthUpToUnrealMatrix"))
  FMatrix
  ComputeEastSouthUpAtEarthCenteredEarthFixedPositionToUnrealTransformation(
      const FVector& EarthCenteredEarthFixedPosition) const;

  /**
   * Computes the matrix that transforms from the Unreal frame to an
   * East-South-Up frame centered at a given location. The location is expressed
   * in Unreal coordinates. To use an Earth-Centered, Earth-Fixed position
   * instead, use
   * ComputeEastSouthUpAtEarthCenteredEarthFixedPositionToUnrealTransformation.
   *
   * In an East-South-Up frame, +X points East, +Y points South, and +Z points
   * Up. However, the directions of "East", "South", and "Up" in Unreal or ECEF
   * coordinates vary depending on where on the globe we are talking about.
   * That is why this function takes a location, expressed in Unreal
   * coordinates, that defines the origin of the East-South-Up frame of
   * interest.
   *
   * The Unreal location and the resulting matrix should generally not be
   * relative to the Unreal _world_, but rather be expressed in some parent
   * Actor's reference frame as defined by its Transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta = (ReturnDisplayName = "UnrealToEastSouthUpMatrix"))
  FMatrix
  ComputeUnrealToEastSouthUpTransformation(const FVector& UnrealLocation) const;

#pragma endregion

#pragma region Editor Support

#if WITH_EDITOR
public:
  /**
   * Places the georeference origin at the camera's current location. Rotates
   * the globe so the current longitude/latitude/height of the camera is at the
   * Unreal origin. The camera is also teleported to the new Unreal origin and
   * rotated so that the view direction is maintained.
   *
   * Warning: Before clicking, ensure that all non-Cesium objects in the
   * persistent level are georeferenced with the "CesiumGlobeAnchorComponent"
   * or attached to an actor with that component. Ensure that static actors only
   * exist in georeferenced sub-levels.
   */
  UFUNCTION(Category = "Cesium")
  void PlaceGeoreferenceOriginHere();

  /**
   * Creates a new Level Instance Actor at the current viewport location, and
   * attaches the Cesium Sub Level Component to it. You will be prompted for
   * where to store the new level.
   *
   * Warning: Before clicking, ensure that all non-Cesium objects in the
   * persistent level are georeferenced with the "CesiumGlobeAnchorComponent"
   * or attached to an actor with that component. Ensure that static actors only
   * exist in georeferenced sub-levels.
   */
  UFUNCTION(Category = "Cesium")
  void CreateSubLevelHere();
#endif

private:
  // This property mirrors RootComponent, and exists only so that the root
  // component's transform is editable in the Editor.
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  USceneComponent* Root;

#if WITH_EDITOR
  /**
   * @brief Show the load radius of each sub-level as a sphere.
   *
   * If this is not called "in-game", and `ShowLoadRadii` is `true`,
   * then it will show a sphere indicating the load radius of each
   * sub-level.
   */
  void _showSubLevelLoadRadii() const;
#endif

#pragma endregion

#pragma region Unreal Lifecycle

protected:
  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;
  virtual void Serialize(FArchive& Ar) override;
  virtual void BeginPlay() override;
  virtual void OnConstruction(const FTransform& Transform) override;
  virtual void PostLoad() override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#pragma endregion

#pragma region Obsolete

public:
  UE_DEPRECATED(
      "Cesium For Unreal v2.0",
      "Use transformation functions on ACesiumGeoreference and UCesiumEllipsoid instead.")
  GeoTransforms GetGeoTransforms() const noexcept;

private:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  UPROPERTY(
      Meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Create sub-levels by adding a UCesiumSubLevelComponent to an ALevelInstance Actor."))
  TArray<FCesiumSubLevel> CesiumSubLevels_DEPRECATED;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

#if WITH_EDITOR
  void _createSubLevelsFromWorldComposition();
#endif

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) into
   * Earth-Centered, Earth-Fixed (ECEF) coordinates.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed on UCesiumEllipsoid instead."))
  FVector TransformLongitudeLatitudeHeightToEcef(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * Ellipsoid longitude in degrees (x), latitude in degrees (y), and height
   * above the ellipsoid in meters (z).
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight on UCesiumEllipsoid instead."))
  FVector TransformEcefToLongitudeLatitudeHeight(const FVector& Ecef) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium",
      meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use EastNorthUpToEllipsoidCenteredEllipsoidFixed on UCesiumEllipsoid instead."))
  FMatrix ComputeEastNorthUpToEcef(const FVector& Ecef) const;

#pragma endregion

private:
#pragma region Implementation Details

public:
  ACesiumGeoreference();

  const CesiumGeospatial::LocalHorizontalCoordinateSystem&
  GetCoordinateSystem() const noexcept {
    return this->_coordinateSystem;
  }

private:
  /**
   * Recomputes all world georeference transforms.
   */
  void UpdateGeoreference();

  /**
   * A tag that is assigned to Georeferences when they are created
   * as the "default" Georeference for a certain world.
   */
  static FName DEFAULT_GEOREFERENCE_TAG;

  CesiumGeospatial::LocalHorizontalCoordinateSystem _coordinateSystem{
      glm::dmat4(1.0)};

  /**
   * Updates _geoTransforms based on the current ellipsoid and center, and
   * returns the old transforms.
   */
  void _updateCoordinateSystem();

  friend class FCesiumGeoreferenceCustomization;
#pragma endregion
};
