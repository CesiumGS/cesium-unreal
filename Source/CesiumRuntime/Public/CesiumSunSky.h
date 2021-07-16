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

UCLASS()
class CESIUMRUNTIME_API ACesiumSunSky : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  ACesiumSunSky();

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  USceneComponent* Scene;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  UStaticMeshComponent* CompassMesh;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  USkyLightComponent* SkyLight;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  UDirectionalLightComponent* DirectionalLight;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Components)
  USkyAtmosphereComponent* SkyAtmosphereComponent;

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cesium)
  ACesiumGeoreference* Georeference;

  /**
   * Updates the atmosphere automatically given current player pawn's longitude,
   * latitude, and height. Fixes artifacts seen with the atmosphere rendering
   * when flying high above the surface, or low to the ground in high latitudes.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cesium)
  bool UpdateAtmosphereAtRuntime = true;

  /**
   * How frequently the atmosphere should be updated, in seconds.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cesium)
  float UpdateAtmospherePeriod = 1.f;

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
   * Sun elevation
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Sun)
  float Elevation = 0.f;

  /**
   * Sun elevation, corrected for atmospheric diffraction
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Sun)
  float CorrectedElevation = 0.f;

  /**
   * Sun azimuth
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Sun)
  float Azimuth = 0.f;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = Location,
      meta =
          (UIMin = -89.99, UIMax = 89.99, ClampMin = -89.99, ClampMax = 89.99))
  float Latitude = 45.f;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = Location,
      meta = (UIMin = -180, UIMax = 180, ClampMin = -180, ClampMax = 180))
  float Longitude = -73.f;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = Location,
      meta = (UIMin = -12, UIMax = 14, ClampMin = -12, ClampMax = 14))
  float TimeZone = -5.f;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = Location,
      meta = (UIMin = -360, UIMax = 360, ClampMin = -360, ClampMax = 360))
  float NorthOffset = -90.f;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Date and Time",
      meta = (UIMin = -4, UIMax = 22, ClampMin = 0, ClampMax = 23.9999))
  float SolarTime = 13.f;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Date and Time",
      meta = (UIMin = 1, UIMax = 31, ClampMin = 1, ClampMax = 31))
  int32 Day = 21;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Date and Time",
      meta = (UIMin = 1, UIMax = 12, ClampMin = 1, ClampMax = 12))
  int32 Month = 9;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Date and Time",
      meta = (UIMin = 1800, UIMax = 2200, ClampMin = 0, ClampMax = 4000))
  int32 Year = 2019;

  /**
   * Enables Daylight Saving Time (DST)
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time")
  bool UseDaylightSavingTime = true;

  /**
   * Set the Date at which DST starts in the current year
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (UIMin = 1, UIMax = 12, ClampMin = 1, ClampMax = 12))
  int32 DSTStartMonth = 3;

  /**
   * Set the Date at which DST starts in the current year
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (UIMin = 1, UIMax = 31, ClampMin = 1, ClampMax = 31))
  int32 DSTStartDay = 10;

  /**
   * Set the Date at which DST ends in the current year
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (UIMin = 1, UIMax = 12, ClampMin = 1, ClampMax = 12))
  int32 DSTEndMonth = 11;

  /**
   * Set the Date at which DST ends in the current year
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (UIMin = 1, UIMax = 31, ClampMin = 1, ClampMax = 31))
  int32 DSTEndDay = 3;

  /**
   * Hour of the DST Switch for both beginning and end
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      AdvancedDisplay,
      Category = "Date and Time",
      meta = (UIMin = 0, UIMax = 23, ClampMin = 0, ClampMax = 23))
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
  UPROPERTY(EditAnywhere, Category = Mobile)
  float MobileDirectionalLightIntensity = 6.f;

  /**
   * Determines whether the date and sun settings have changed and warrant
   * a refresh of the SkyAtmosphere rendering.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Event Tick")
  float HashVal;

public:
  UFUNCTION(
      CallInEditor,
      BlueprintCallable,
      BlueprintNativeEvent,
      Category = Sun)
  void UpdateSun();
  void UpdateSun_Implementation();

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
   * Callback after georeference origin (e.g. lat/long position) has been
   * updated. Sets this actor's position to the Earth's center.
   */
  UFUNCTION(BlueprintCallable, Category = Cesium)
  void HandleGeoreferenceUpdated();

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

#if WITH_EDITOR
  virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
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
};
