// Fill out your copyright notice in the Description page of Project Settings.

#include "CesiumSunSky.h"
#include "CesiumRuntime.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ACesiumSunSky::ACesiumSunSky() {
  PrimaryActorTick.bCanEverTick = false;

  Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
  SetRootComponent(Scene);

  CompassMesh = CreateDefaultSubobject<UStaticMeshComponent>(
      TEXT("CompassMesh"));
  CompassMesh->SetupAttachment(Scene);
  ConstructorHelpers::FObjectFinder<UStaticMesh> CompassFinder(
      TEXT("Class'/SunPosition/Editor/SM_Compass'"));
  if (CompassFinder.Succeeded()) {
    CompassMesh->SetStaticMesh(CompassFinder.Object);
  }
  CompassMesh->SetCollisionProfileName(TEXT("NoCollision"));
  CompassMesh->CastShadow = false;
  CompassMesh->bHiddenInGame = true;
  CompassMesh->bIsEditorOnly = true;

  SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
  SkyLight->SetupAttachment(Scene);
  SkyLight->SetRelativeLocation(FVector(0, 0, 150));
  SkyLight->SetMobility(EComponentMobility::Movable);
  SkyLight->bRealTimeCapture = true;
  SkyLight->bLowerHemisphereIsBlack = false;
  SkyLight->bTransmission = true;
  SkyLight->bCastRaytracedShadow = true;
  SkyLight->SamplesPerPixel = 2;

  DirectionalLight = CreateDefaultSubobject<UDirectionalLightComponent>(
      TEXT("DirectionalLight"));
  DirectionalLight->SetupAttachment(Scene);
  DirectionalLight->SetRelativeLocation(FVector(0, 0, 100));
  DirectionalLight->Intensity = 111000.f;
  DirectionalLight->LightSourceAngle = 0.5;
  DirectionalLight->bUsedAsAtmosphereSunLight = true;
  DirectionalLight->DynamicShadowCascades = 5;
  DirectionalLight->CascadeDistributionExponent = 1.4;

  SkyAtmosphereComponent = CreateDefaultSubobject<USkyAtmosphereComponent>(
      TEXT("SkyAtmosphere"));
  SkyAtmosphereComponent->SetupAttachment(Scene);
  SkyAtmosphereComponent->TransformMode =
      ESkyAtmosphereTransformMode::PlanetCenterAtComponentTransform;

  if (!Georeference) {
    Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  if (Georeference) {
    Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
        this,
        &ACesiumSunSky::HandleGeoreferenceUpdated);
  }
}

void ACesiumSunSky::UpdateSun_Implementation() {
  // No C++ base implementation for now
}

void ACesiumSunSky::GetHMSFromSolarTime(
    float InSolarTime,
    int32& Hour,
    int32& Minute,
    int32& Second) {
  Hour = FMath::TruncToInt(InSolarTime) % 24;
  Minute = (FMath::TruncToInt(InSolarTime - Hour) * 60) % 60;
  
  // Convert hours + minutes so far to seconds, subtract from InSolarTime.
  // Not exactly sure why 0.5 is added at the end. Maybe to ensure that times
  // on the hour (e.g. 13.0) don't get truncated down after arithmetic
  // due to floating point errors.
  float secondsFloat = ((InSolarTime - Hour - Minute / 60) * 3600) + 0.5;
  Second = FMath::TruncToInt(secondsFloat) % 60;
}

bool ACesiumSunSky::IsDST(
    bool DSTEnable,
    int32 InDSTStartMonth,
    int32 InDSTStartDay,
    int32 InDSTEndMonth,
    int32 InDSTEndDay,
    int32 InDSTSwitchHour) const {
  if (!DSTEnable) {
    return false;
  }
  int32 hour, minute, second;
  this->GetHMSFromSolarTime(SolarTime, hour, minute, second);
  
  // Editor will crash if we create an invalid FDateTime, so validate these
  // settings first
  if (!FDateTime::Validate(Year, Month, Day, hour, minute, second, 0)) {
    return false;
  }
  FDateTime current = FDateTime(Year, Month, Day, hour, minute, second);
  FDateTime dstStart = FDateTime(
      Year,
      InDSTStartMonth,
      InDSTStartDay,
      InDSTSwitchHour);
  FDateTime dstEnd = FDateTime(
      Year,
      InDSTEndMonth,
      InDSTEndDay,
      InDSTSwitchHour);
  return current >= dstStart && current <= dstEnd;
}

void ACesiumSunSky::HandleGeoreferenceUpdated() {
  if (Georeference) {
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("HandleGeoreferenceUpdated entered on CesiumSunSky"));
    this->SetActorLocation(
        Georeference->InaccurateTransformEcefToUnreal(FVector::ZeroVector));
    switch (Georeference->OriginPlacement) {
    case EOriginPlacement::CartographicOrigin: {
      FVector llh = Georeference->
          InaccurateGetGeoreferenceOriginLongitudeLatitudeHeight();
      this->Longitude = llh.X;
      this->Latitude = llh.Y;
      UpdateSun();
      break;
    }
    default:
      break;
    }
  }
}

void ACesiumSunSky::SetSkyAtmosphereGroundRadius(
    USkyAtmosphereComponent* Sky,
    float Radius) {
  // Only update if there's a significant change to be made
  if (Sky && FMath::Abs(Sky->BottomRadius - Radius) > 0.1) {
    Sky->BottomRadius = Radius;
    Sky->MarkRenderStateDirty();
    UE_LOG(LogCesium, Verbose, TEXT("GroundRadius now %f"), Sky->BottomRadius);
  }
}
