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
class UCesiumSubLevelSwitcherComponent;

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
  /**
   * Finds and returns the actor labeled `CesiumGeoreferenceDefault` in the
   * persistent level of the calling object's world. If not found, it creates a
   * new default Georeference.
   * @param WorldContextObject Any `UObject`.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (WorldContext = "WorldContextObject"))
  static ACesiumGeoreference*
  GetDefaultGeoreference(const UObject* WorldContextObject);

  /**
   * A delegate that will be called whenever the Georeference is
   * modified in a way that affects its computations.
   */
  UPROPERTY(BlueprintAssignable, Category = "Cesium")
  FGeoreferenceUpdated OnGeoreferenceUpdated;

#pragma region Properties

private:
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
   */
  UPROPERTY(
      Category = "Cesium|Sub-levels",
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetSubLevelCamera,
      BlueprintSetter = SetSublevelCamera,
      meta = (AllowPrivateAccess))
  APlayerCameraManager* SubLevelCamera = nullptr;

#if WITH_EDITORONLY_DATA
  /*
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
  FVector GetGeoreferenceOriginLongitudeLatitudeHeight() const;

  /**
   * This aligns the specified longitude in degrees (X), latitude in
   * degrees (Y), and height above the ellipsoid in meters (Z) to Unreal's world
   * origin. That is, it moves the globe so that these coordinates exactly fall
   * on the origin.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetGeoreferenceOriginLongitudeLatitudeHeight(
      const FVector& TargetLongitudeLatitudeHeight);

  /**
   * This aligns the specified Earth-Centered, Earth-Fixed (ECEF) coordinates to
   * Unreal's world origin. I.e. it moves the globe so that these coordinates
   * exactly fall on the origin.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetGeoreferenceOriginEcef(const FVector& TargetEcef);

  UFUNCTION(BlueprintGetter)
  EOriginPlacement GetOriginPlacement() const;

  UFUNCTION(BlueprintSetter)
  void SetOriginPlacement(EOriginPlacement NewValue);

  UFUNCTION(BlueprintGetter)
  double GetOriginLatitude() const;

  UFUNCTION(BlueprintSetter)
  void SetOriginLatitude(double NewValue);

  UFUNCTION(BlueprintGetter)
  double GetOriginLongitude() const;

  UFUNCTION(BlueprintSetter)
  void SetOriginLongitude(double NewValue);

  UFUNCTION(BlueprintGetter)
  double GetOriginHeight() const;

  UFUNCTION(BlueprintSetter)
  void SetOriginHeight(double NewValue);

  /**
   * The percentage scale of the globe in the Unreal world. If this value is 50,
   * for example, one meter on the globe occupies half a meter in the Unreal
   * world.
   */
  UFUNCTION(BlueprintGetter)
  double GetScale() const;

  /**
   * The percentage scale of the globe in the Unreal world. If this value is 50,
   * for example, one meter on the globe occupies half a meter in the Unreal
   * world.
   */
  UFUNCTION(BlueprintSetter)
  void SetScale(double NewValue);

  UFUNCTION(BlueprintGetter)
  APlayerCameraManager* GetSubLevelCamera() const;

  UFUNCTION(BlueprintSetter)
  void SetSubLevelCamera(APlayerCameraManager* NewValue);

  UFUNCTION(BlueprintGetter)
  bool GetShowLoadRadii() const;

  UFUNCTION(BlueprintSetter)
  void SetShowLoadRadii(bool NewValue);

#pragma endregion

#pragma region Transformation Functions

public:
  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) into
   * Earth-Centered, Earth-Fixed (ECEF) coordinates.
   *
   * This function peforms the computation in single-precision. When using
   * the C++ API, corresponding double-precision function
   * TransformLongitudeLatitudeHeightToEcef can be used.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector TransformLongitudeLatitudeHeightToEcef(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height above
   * the ellipsoid in meters (z).
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector TransformEcefToLongitudeLatitudeHeight(const FVector& Ecef) const;

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z) into Unreal
   * coordinates. The resulting position should generally not be interpreted as
   * an Unreal _world_ position, but rather a position expressed in some parent
   * Actor's reference frame as defined by its Transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector TransformLongitudeLatitudeHeightToUnreal(
      const FVector& LongitudeLatitudeHeight) const;

  /**
   * Transforms Unreal coordinates into longitude in degrees (x), latitude in
   * degrees (y), and height above the ellipsoid in meters (z). The position
   * should generally not be an Unreal _world_ position, but rather a position
   * expressed in some parent Actor's reference frame as defined by its
   * Transform. This way, the chain of Unreal transforms places and orients the
   * "globe" in the Unreal world.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector TransformUnrealToLongitudeLatitudeHeight(const FVector& Unreal) const;

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal coordinates. The resulting position should generally not be
   * interpreted as an Unreal _world_ position, but rather a position expressed
   * in some parent Actor's reference frame as defined by its Transform. This
   * way, the chain of Unreal transforms places and orients the "globe" in the
   * Unreal world.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector TransformEcefToUnreal(const FVector& Ecef) const;

  /**
   * Transforms the given point from Unreal coordinates to Earth-Centered,
   * Earth-Fixed (ECEF). The position should generally not be an Unreal _world_
   * position, but rather a position expressed in some parent Actor's reference
   * frame as defined by its Transform. This way, the chain of Unreal transforms
   * places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FVector TransformUnrealToEcef(const FVector& Unreal) const;

  /**
   * Transforms a rotator from Unreal to East-South-Up at the given
   * Unreal location. The rotator and location should generally not be relative
   * to the Unreal _world_, but rather be expressed in some parent
   * Actor's reference frame as defined by its Transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FRotator TransformRotatorUnrealToEastSouthUp(
      const FRotator& UnrealRotator,
      const FVector& UnrealLocation) const;

  /**
   * Transforms a rotator from East-South-Up to Unreal at the given
   * Unreal location. The location and resulting rotator should generally not be
   * relative to the Unreal _world_, but rather be expressed in some parent
   * Actor's reference frame as defined by its Transform. This way, the chain of
   * Unreal transforms places and orients the "globe" in the Unreal world.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FRotator TransformRotatorEastSouthUpToUnreal(
      const FRotator& EsuRotator,
      const FVector& UnrealLocation) const;

  /**
   * Computes the rotation matrix from the local East-South-Up to Unreal at the
   * specified Unreal location. The returned transformation works in Unreal's
   * left-handed coordinate system. The location and resulting rotation should
   * generally not be relative to the Unreal _world_, but rather be expressed in
   * some parent Actor's reference frame as defined by its Transform. This way,
   * the chain of Unreal transforms places and orients the "globe" in the Unreal
   * world.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FMatrix ComputeEastSouthUpToUnreal(const FVector& Unreal) const;

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium")
  FMatrix ComputeEastNorthUpToEcef(const FVector& Ecef) const;

#pragma endregion

#pragma region Editor Support

public:
#if WITH_EDITOR
  /**
   * Places the georeference origin at the camera's current location. Rotates
   * the globe so the current longitude/latitude/height of the camera is at the
   * Unreal origin. The camera is also teleported to the Unreal origin.
   *
   * Warning: Before clicking, ensure that all non-Cesium objects in the
   * persistent level are georeferenced with the "CesiumGeoreferenceComponent"
   * or attached to an actor with that component. Ensure that static actors only
   * exist in georeferenced sub-levels.
   */
  UFUNCTION(CallInEditor, Category = "Actions")
  void PlaceGeoreferenceOriginHere();
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
  virtual void BeginDestroy() override;
  virtual void PostLoad() override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

#pragma endregion

#pragma region Obsolete

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

#pragma endregion

#pragma region Implementation Details

public:
  ACesiumGeoreference();

  /**
   * Recomputes all world georeference transforms. Usually there is no need to
   * explicitly call this from external code.
   */
  void UpdateGeoreference();

  /**
   * Returns the GeoTransforms that offers the same conversion
   * functions as this class, but performs the computations
   * in double precision.
   */
  const GeoTransforms& GetGeoTransforms() const noexcept {
    return _geoTransforms;
  }

private:
  /**
   * A tag that is assigned to Georeferences when they are created
   * as the "default" Georeference for a certain world.
   */
  static FName DEFAULT_GEOREFERENCE_TAG;

  GeoTransforms _geoTransforms;

  UPROPERTY()
  UCesiumSubLevelSwitcherComponent* SubLevelSwitcher;

  // TODO: add option to set georeference directly from ECEF
  void _setGeoreferenceOrigin(
      double targetLongitude,
      double targetLatitude,
      double targetHeight);

  /**
   * @brief Updates the load state of sub-levels.
   *
   * This checks all sub-levels whether their load radius contains the
   * `SubLevelCamera`, in ECEF coordinates. The sub-levels that
   * contain the camera will be loaded. All others will be unloaded.
   *
   * @return Whether the camera is contained in *any* sub-level.
   */
  bool _updateSublevelState();

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

  /**
   * Determines if this Georeference should manage sub-level switching.
   *
   * A Georeference inside a sub-level should not manage sub-level switching,
   * so this function returns true the Georeference is in the world's
   * PersistentLevel.
   */
  bool _shouldManageSubLevels() const;
#pragma endregion
};
