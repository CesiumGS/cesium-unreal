// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include "CesiumGeoreference.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/Actor.h"
#include "CesiumSunSky.generated.h"

class UCesiumGlobeAnchorComponent;

/**
 * A globe-aware sun sky actor. If the georeference is set to CartographicOrigin
 * (aka Longitude/Latitude/Height) mode, then this actor will automatically
 * sync its longitude and latitude properties with the georeference's, and
 * recalculate the sun position whenever those properties change.
 *
 * Note: because we use `Planet Center at Component Transform`
 * for the SkyAtmosphere transform mode, this actor's location will be forced
 * to the Earth's center if the georeference is set to CartographicOrigin.
 */
UCLASS()
class CESIUMRUNTIME_API ACesiumSunSky : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  ACesiumSunSky();

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  USceneComponent* Scene;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  USkyLightComponent* SkyLight;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  UDirectionalLightComponent* DirectionalLight;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  USkyAtmosphereComponent* SkyAtmosphere;

  /**
   * The Globe Anchor Component that precisely ties this Actor to the Globe.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  UCesiumGlobeAnchorComponent* GlobeAnchor;

  /**
   * Gets the time zone, represented as hours offset from GMT.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time",
      meta = (ClampMin = -12, ClampMax = 14))
  double TimeZone = -5.0;

  /**
   * The current solar time represented as hours from midnight.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time",
      meta = (UIMin = 4, UIMax = 22, ClampMin = 0, ClampMax = 23.9999))
  double SolarTime = 13.0;

  /**
   * The day of the month.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time",
      meta = (ClampMin = 1, ClampMax = 31))
  int32 Day = 21;

  /**
   * The month of the year, where 1 is January and 12 is December.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time",
      meta = (ClampMin = 1, ClampMax = 12))
  int32 Month = 9;

  /**
   * The year.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time",
      meta = (UIMin = 1800, UIMax = 2200, ClampMin = 0, ClampMax = 4000))
  int32 Year = 2019;

  /**
   * Offset in the sun's position. Should be set to -90 for the sun's position
   * to be accurate in the Unreal reference frame.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      BlueprintReadWrite,
      Category = "Cesium|Date and Time",
      meta = (ClampMin = -360, ClampMax = 360))
  double NorthOffset = -90.0;

  /**
   * Enables adjustment of the Solar Time for Daylight Saving Time (DST).
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time|Daylight Savings")
  bool UseDaylightSavingTime = true;

protected:
  /**
   * THIS PROPERTY IS DEPRECATED.
   *
   * Get the Georeference instance from the Globe Anchor Component instead.
   */
  UPROPERTY(
      BlueprintReadOnly,
      Category = "Cesium",
      BlueprintGetter = GetGeoreference,
      Meta =
          (DeprecatedProperty,
           DeprecationMessage =
               "Get the Georeference instance from the Globe Anchor Component instead."))
  ACesiumGeoreference* Georeference_DEPRECATED;

  /**
   * Gets the Georeference Actor associated with this instance. It is obtained
   * from the Globe Anchor Component.
   */
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  ACesiumGeoreference* GetGeoreference() const;

  /**
   * Set the Date at which DST starts in the current year.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time|Daylight Savings",
      meta = (ClampMin = 1, ClampMax = 12),
      meta = (EditCondition = "UseDaylightSavingTime"))
  int32 DSTStartMonth = 3;

  /**
   * Set the Date at which DST starts in the current year.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time|Daylight Savings",
      meta = (ClampMin = 1, ClampMax = 31),
      meta = (EditCondition = "UseDaylightSavingTime"))
  int32 DSTStartDay = 10;

  /**
   * Set the Date at which DST ends in the current year.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time|Daylight Savings",
      meta = (ClampMin = 1, ClampMax = 12),
      meta = (EditCondition = "UseDaylightSavingTime"))
  int32 DSTEndMonth = 11;

  /**
   * Set the Date at which DST ends in the current year.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time|Daylight Savings",
      meta = (ClampMin = 1, ClampMax = 31),
      meta = (EditCondition = "UseDaylightSavingTime"))
  int32 DSTEndDay = 3;

  /**
   * Hour of the DST Switch for both beginning and end.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Date and Time|Daylight Savings",
      meta = (ClampMin = 0, ClampMax = 23),
      meta = (EditCondition = "UseDaylightSavingTime"))
  int32 DSTSwitchHour = 2;

  /**
   * Updates the atmosphere automatically given current player pawn's longitude,
   * latitude, and height. Fixes artifacts seen with the atmosphere rendering
   * when flying high above the surface, or low to the ground in high latitudes.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Atmosphere")
  bool UpdateAtmosphereAtRuntime = true;

  /**
   * When the player pawn is below this height, which is expressed in kilometers
   * above the ellipsoid, this Actor will use an atmosphere ground radius that
   * is calculated to be at or below the terrain surface at the player pawn's
   * current location. This avoids a gap between the bottom of the atmosphere
   * and the top of the terrain when zoomed in close to the terrain surface.
   *
   * Above CircumscribedGroundThreshold, this Actor will use an atmosphere
   * ground radius that is guaranteed to be above the terrain surface anywhere
   * on Earth. This avoids artifacts caused by terrain poking through the
   * atmosphere when viewing the Earth from far away.
   *
   * At player pawn heights in between InscribedGroundThreshold and
   * CircumscribedGroundThreshold, this Actor uses a linear interpolation
   * between the two ground radii.
   *
   * This value is automatically scaled according to the CesiumGeoreference
   * Scale and the Actor scale.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      meta = (EditCondition = "UpdateAtmosphereAtRuntime"),
      Category = "Cesium|Atmosphere")
  double InscribedGroundThreshold = 30.0;

  /**
   * When the player pawn is above this height, which is expressed in kilometers
   * above the ellipsoid, this Actor will use an atmosphere ground radius that
   * is guaranteed to be above the terrain surface anywhere on Earth. This
   * avoids artifacts caused by terrain poking through the atmosphere when
   * viewing the Earth from far away.
   *
   * Below InscribedGroundThreshold, this Actor will use an atmosphere
   * ground radius that is calculated to be at or below the terrain surface at
   * the player pawn's current location. This avoids a gap between the bottom of
   * the atmosphere and the top of the terrain when zoomed in close to the
   * terrain surface.
   *
   * At heights in between InscribedGroundThreshold and
   * CircumscribedGroundThreshold, this Actor uses a linear interpolation
   * between the two ground radii.
   *
   * This value is automatically scaled according to the CesiumGeoreference
   * Scale and the Actor scale.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      meta = (EditCondition = "UpdateAtmosphereAtRuntime"),
      Category = "Cesium|Atmosphere")
  double CircumscribedGroundThreshold = 100.0;

  /**
   * The height at which to place the bottom of the atmosphere when the player
   * pawn is above the CircumscribedGroundThreshold. This is expressed as a
   * height in kilometers above the maximum radius of the ellipsoid (usually
   * WGS84). To avoid dark splotchy artifacts in the atmosphere when zoomed out
   * far from the globe, this value must be above the greatest height achieved
   * by any part of the tileset.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      meta = (EditCondition = "UpdateAtmosphereAtRuntime"),
      Category = "Cesium|Atmosphere")
  double CircumscribedGroundHeight = 0.0;

  /**
   * The height of the atmosphere layer above the ground, in kilometers. This
   * value is automatically scaled according to the CesiumGeoreference Scale and
   * the Actor scale. However, Unreal Engine's SkyAtmosphere has a hard-coded
   * minimum effective value of 0.1, so the atmosphere will look too thick when
   * the globe is scaled down drastically.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      interp,
      Category = "Cesium|Atmosphere",
      meta = (UIMin = 1.0, UIMax = 200.0, ClampMin = 0.1, SliderExponent = 2.0))
  float AtmosphereHeight = 60.0f;

  /**
   * Makes the aerial perspective look thicker by scaling distances from view
   * to surfaces (opaque and translucent). This value is automatically scaled
   * according to the CesiumGeoreference Scale and the Actor scale.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      interp,
      Category = "Cesium|Atmosphere",
      meta =
          (DisplayName = "Aerial Perspective View Distance Scale",
           UIMin = 0.0,
           UIMax = 3.0,
           ClampMin = 0.0,
           SliderExponent = 2.0))
  float AerialPerspectiveViewDistanceScale = 1.0f;

  /**
   * The altitude in kilometers at which Rayleigh scattering effect is reduced
   * to 40%. This value is automatically scaled according to the
   * CesiumGeoreference Scale and the Actor scale.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      interp,
      Category = "Cesium|Atmosphere",
      meta =
          (UIMin = 0.01, UIMax = 20.0, ClampMin = 0.001, SliderExponent = 5.0))
  float RayleighExponentialDistribution = 8.0f;

  /**
   * The altitude in kilometers at which Mie effects are reduced to 40%. This
   * value is automatically scaled according to the CesiumGeoreference Scale and
   * the Actor scale.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      interp,
      Category = "Cesium|Atmosphere",
      meta =
          (UIMin = 0.01, UIMax = 10.0, ClampMin = 0.001, SliderExponent = 5.0))
  float MieExponentialDistribution = 1.2f;

  /**
   * False: Use Directional Light component inside CesiumSunSky.
   * True: Use the assigned Directional Light in the level.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Sun")
  bool UseLevelDirectionalLight = false;

  /**
   * Reference to a manually assigned Directional Light in the level.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Sun")
  ADirectionalLight* LevelDirectionalLight;

  /**
   * The current sun elevation in degrees above the horizontal, as viewed from
   * the Georeference origin.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun")
  double Elevation = 0.f;

  /**
   * The current sun elevation, corrected for atmospheric diffraction, in
   * degrees above the horizontal, as viewed from the Georeference origin.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun")
  double CorrectedElevation = 0.f;

  /**
   * The current sun azimuth in degrees clockwise from North toward East, as
   * viewed from the Georeference origin.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun")
  double Azimuth = 0.f;

  /**
   * A switch to toggle between desktop and mobile rendering code paths.
   * This will NOT be automatically set when running on mobile, so make sure
   * to check this setting before building on mobile platforms.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cesium|Mobile")
  bool UseMobileRendering;

  /**
   * Mobile platforms currently do not support the SkyAtmosphereComponent.
   * In lieu of that, use the engine BP_Sky_Sphere class, or a derived class.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Mobile")
  TSubclassOf<AActor> SkySphereClass;

  /**
   * Reference to BP_Sky_Sphere or similar actor (mobile only)
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cesium|Mobile")
  AActor* SkySphereActor;

  /**
   * Default intensity of directional light that's spawned for mobile rendering.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cesium|Mobile")
  double MobileDirectionalLightIntensity = 6.f;

public:
  UFUNCTION(
      CallInEditor,
      BlueprintCallable,
      BlueprintNativeEvent,
      Category = "Cesium")
  void UpdateSun();
  void UpdateSun_Implementation();

  UFUNCTION(CallInEditor, BlueprintCallable, Category = "Cesium")
  void UpdateAtmosphereRadius();

  /**
   * Adjusts the time zone of this CesiumSunSky to an estimate based on the
   * given longitude.
   *
   * The time zone is naively calculated from the longitude, where every
   * 15 degrees equals 1 hour. This may not necessarily match the official
   * time zone at a given location within that longitude.
   *
   * This method will call @ref UpdateSun automatically.
   *
   * @param InLongitude The longitude that the calculated time zone will be
   * based on in degrees in the range [-180, 180].
   */
  UFUNCTION(CallInEditor, BlueprintCallable, Category = "Cesium")
  void EstimateTimeZoneForLongitude(double InLongitude);

  /**
   * Convert solar time to Hours:Minutes:Seconds. Copied the implementation
   * from the engine SunSkyBP class.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = Sun)
  static void GetHMSFromSolarTime(
      double InSolarTime,
      int32& Hour,
      int32& Minute,
      int32& Second);

  /**
   * Check whether the current time and date (based on this class instance's
   * properties) falls within Daylight Savings Time. Copied the implementation
   * from the engine SunSkyBP class.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = Sun)
  bool IsDST(
      bool DSTEnable,
      int32 InDSTStartMonth,
      int32 InDSTStartDay,
      int32 InDSTEndMonth,
      int32 InDSTEndDay,
      int32 InDSTSwitchHour) const;

  // Begin AActor Interface
  /**
   * Gets called when the actor is first created, and when properties are
   * changed at edit-time. Refreshes the actor's position w/r/t the georeference
   * and handles mobile-specific setup if needed.
   */
  virtual void OnConstruction(const FTransform& Transform) override;
  // End AActor Interface

protected:
  /**
   * Modifies the sky atmosphere's ground radius, which represents the Earth's
   * radius in the SkyAtmosphere rendering model. Only changes if there's a >0.1
   * difference, to reduce redraws.
   *
   * @param Sky A pointer to the SkyAtmosphereComponent
   * @param Radius The radius in kilometers.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void
  SetSkyAtmosphereGroundRadius(USkyAtmosphereComponent* Sky, double Radius);

  /**
   * Update MobileSkySphere by calling its RefreshMaterial function
   */
  UFUNCTION(BlueprintCallable, Category = "Mobile")
  void UpdateSkySphere();

  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void Serialize(FArchive& Ar) override;
  virtual void Tick(float DeltaSeconds) override;
  virtual void PostLoad() override;
  virtual bool ShouldTickIfViewportsOnly() const override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
  void _spawnSkySphere();
  double _computeScale() const;

  // Sets Directional Light Component in Sky Sphere actor
  void _setSkySphereDirectionalLight();

  void _setSkyAtmosphereVisibility(bool bVisible);

  // Determines whether mobile sky sphere will be spawned during OnConstruction.
  bool _wantsSpawnMobileSkySphere;

  void _handleTransformUpdated(
      USceneComponent* InRootComponent,
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport);

  FDelegateHandle _transformUpdatedSubscription;
};
