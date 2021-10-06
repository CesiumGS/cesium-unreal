// Copyright 2020-2021 CesiumGS, Inc. and Contributors

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
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  UCesiumGlobeAnchorComponent* GlobeAnchor;

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
   * Updates the atmosphere automatically given current player pawn's longitude,
   * latitude, and height. Fixes artifacts seen with the atmosphere rendering
   * when flying high above the surface, or low to the ground in high latitudes.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cesium)
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
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cesium)
  float InscribedGroundThreshold = 30;

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
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cesium)
  float CircumscribedGroundThreshold = 100;

  /**
   * False: Use Directional Light component inside CesiumSunSky.
   * True: Use the assigned Directional Light in the level.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
  bool UseLevelDirectionalLight = false;

  /**
   * Reference to a manually assigned Directional Light in the level.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
  ADirectionalLight* LevelDirectionalLight;

  /**
   * The current sun elevation in degrees above the horizontal, as viewed from
   * the Georeference origin.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Sun)
  float Elevation = 0.f;

  /**
   * The current sun elevation, corrected for atmospheric diffraction, in
   * degrees above the horizontal, as viewed from the Georeference origin.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Sun)
  float CorrectedElevation = 0.f;

  /**
   * The current sun azimuth in degrees clockwise from North toward East, as
   * viewed from the Georeference origin.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Sun)
  float Azimuth = 0.f;

  /**
   * Gets the time zone, represented as hours offset from GMT.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = Location,
      meta = (ClampMin = -12, ClampMax = 14))
  float TimeZone = -5.f;

  /**
   * Offset in the sun's position. Should be set to -90 for the sun's position
   * to be accurate in the Unreal reference frame.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = Location,
      meta = (ClampMin = -360, ClampMax = 360))
  float NorthOffset = -90.f;

  /**
   * The current solar time represented as hours from midnight.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Date and Time",
      meta = (UIMin = 4, UIMax = 22, ClampMin = 0, ClampMax = 23.9999))
  float SolarTime = 13.f;

  /**
   * The day of the month.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Date and Time",
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
      Category = "Date and Time",
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
      Category = "Date and Time",
      meta = (UIMin = 1800, UIMax = 2200, ClampMin = 0, ClampMax = 4000))
  int32 Year = 2019;

  /**
   * Enables adjustment of the Solar Time for Daylight Saving Time (DST).
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time")
  bool UseDaylightSavingTime = true;

  /**
   * Set the Date at which DST starts in the current year.
   *
   * After changing this value from Blueprints or C++, you must call UpdateSun
   * for it to take effect.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (ClampMin = 1, ClampMax = 12))
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
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (ClampMin = 1, ClampMax = 31))
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
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (ClampMin = 1, ClampMax = 12))
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
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (ClampMin = 1, ClampMax = 31))
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
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (ClampMin = 0, ClampMax = 23))
  int32 DSTSwitchHour = 2.f;

  /**
   * A switch to toggle between desktop and mobile rendering code paths.
   * This will NOT be automatically set when running on mobile, so make sure
   * to check this setting before building on mobile platforms.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mobile)
  bool EnableMobileRendering;

  /**
   * Mobile platforms currently do not support the SkyAtmosphereComponent.
   * In lieu of that, use the engine BP_Sky_Sphere class, or a derived class.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mobile)
  TSubclassOf<AActor> SkySphereClass;

  /**
   * Reference to BP_Sky_Sphere or similar actor (mobile only)
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mobile)
  AActor* SkySphereActor;

  /**
   * Default intensity of directional light that's spawned for mobile rendering.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mobile)
  float MobileDirectionalLightIntensity = 6.f;

public:
  UFUNCTION(
      CallInEditor,
      BlueprintCallable,
      BlueprintNativeEvent,
      Category = Sun)
  void UpdateSun();
  void UpdateSun_Implementation();

  UFUNCTION(CallInEditor, BlueprintCallable, Category = "Cesium")
  void AdjustAtmosphereRadius();

  /**
   * Convert solar time to Hours:Minutes:Seconds. Copied the implementation
   * from the engine SunSkyBP class.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = Sun)
  static void GetHMSFromSolarTime(
      float InSolarTime,
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
  UFUNCTION(BlueprintCallable, Category = Cesium)
  void SetSkyAtmosphereGroundRadius(USkyAtmosphereComponent* Sky, float Radius);

  /**
   * Update MobileSkySphere by calling its RefreshMaterial function
   */
  UFUNCTION(BlueprintCallable, Category = Mobile)
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
