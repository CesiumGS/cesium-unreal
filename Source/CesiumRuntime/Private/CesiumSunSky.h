// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "CesiumGeoreference.h"
#include "Components/SkyAtmosphereComponent.h"
#include "GameFramework/Actor.h"
#include "CesiumSunSky.generated.h"

UCLASS()
class CESIUMRUNTIME_API ACesiumSunSky : public AActor {
  GENERATED_BODY()

protected:
  /**
   * Called after the C++ constructor and after the properties have
   * been initialized, including those loaded from config.
   */
  void PostInitProperties() override;

public:
  // Sets default values for this actor's properties
  ACesiumSunSky();

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cesium)
  ACesiumGeoreference* Georeference;

  /**
   * Callback after georeference origin (e.g. lat/long position) has been
   * updated.
   */
  UFUNCTION(BlueprintImplementableEvent)
  void HandleGeoreferenceUpdated();

  /**
   * Modifies the sky atmosphere's ground radius, which represents the Earth's
   * radius in the SkyAtmosphere rendering model. Only changes if there's a >0.1
   * difference, to reduce redraws.
   *
   * @param Sky A pointer to the SkyAtmosphereComponent
   * @param Radius The radius in kilometers.
   */
  UFUNCTION(BlueprintCallable)
  void SetSkyAtmosphereGroundRadius(USkyAtmosphereComponent* Sky, float Radius);
};
