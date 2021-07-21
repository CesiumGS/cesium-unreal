// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumSunSky.h"
#include "CesiumRuntime.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ACesiumSunSky::ACesiumSunSky() {
  PrimaryActorTick.bCanEverTick = false;

  Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
  SetRootComponent(Scene);

  CompassMesh =
      CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CompassMesh"));
  CompassMesh->SetupAttachment(Scene);
  ConstructorHelpers::FObjectFinder<UStaticMesh> compassFinder(
      TEXT("Class'/SunPosition/Editor/SM_Compass'"));
  if (compassFinder.Succeeded()) {
    CompassMesh->SetStaticMesh(compassFinder.Object);
  }
  CompassMesh->SetCollisionProfileName(TEXT("NoCollision"));
  CompassMesh->CastShadow = false;
  CompassMesh->bHiddenInGame = true;
  CompassMesh->bIsEditorOnly = true;

  DirectionalLight = CreateDefaultSubobject<UDirectionalLightComponent>(
      TEXT("DirectionalLight"));
  DirectionalLight->SetupAttachment(Scene);
  DirectionalLight->SetRelativeLocation(FVector(0, 0, 100));
  DirectionalLight->Intensity = 111000.f;
  DirectionalLight->LightSourceAngle = 0.5;
  DirectionalLight->bUsedAsAtmosphereSunLight = true;
  DirectionalLight->DynamicShadowCascades = 5;
  DirectionalLight->CascadeDistributionExponent = 1.4;

  if (!SkySphereClass) {
    ConstructorHelpers::FClassFinder<AActor> skySphereFinder(
        TEXT("Blueprint'/CesiumForUnreal/MobileSkySphere.MobileSkySphere_C'"));
    if (skySphereFinder.Succeeded()) {
      SkySphereClass = skySphereFinder.Class;
    }
  }

  // Always create these components and hide them if not needed (e.g. on mobile)
  SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
  SkyLight->SetupAttachment(Scene);
  SkyLight->SetRelativeLocation(FVector(0, 0, 150));
  SkyLight->SetMobility(EComponentMobility::Movable);
  SkyLight->bRealTimeCapture = true;
  SkyLight->bLowerHemisphereIsBlack = false;
  SkyLight->bTransmission = true;
  SkyLight->bCastRaytracedShadow = true;
  SkyLight->SamplesPerPixel = 2;

  SkyAtmosphereComponent =
      CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
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

void ACesiumSunSky::HandleGeoreferenceUpdated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called HandleGeoreferenceUpdated on CesiumSunSky %s"),
      *this->GetName());
  _updateSunSkyLocation();
}

void ACesiumSunSky::_updateSunSkyLocation() {
  if (!Georeference) {
    return;
  }
  // For mobile, simply set sky sphere to world origin location
  if (EnableMobileRendering) {
    this->SetActorTransform(FTransform::Identity);
  } else {
    this->SetActorLocation(
        Georeference->InaccurateTransformEcefToUnreal(FVector::ZeroVector));
  }

  switch (Georeference->OriginPlacement) {
  case EOriginPlacement::CartographicOrigin: {
    FVector llh =
        Georeference->InaccurateGetGeoreferenceOriginLongitudeLatitudeHeight();
    this->Longitude = llh.X;
    this->Latitude = llh.Y;
    UpdateSun();
    break;
  }
  default:
    break;
  }
}

void ACesiumSunSky::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction(Transform);
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnConstruction for CesiumSunSky %s"),
      *this->GetName());

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Spawn new sky sphere: %s"),
      _wantsSpawnMobileSkySphere ? TEXT("true") : TEXT("false"));
  if (EnableMobileRendering) {
    DirectionalLight->Intensity = MobileDirectionalLightIntensity;
    if (_wantsSpawnMobileSkySphere && SkySphereClass) {
      _spawnSkySphere();
      // UpdateSkySphere();
    }
  }
  _setSkyAtmosphereVisibility(!EnableMobileRendering);

  // Refresh sun locations and sky materials (including mobile)
  _updateSunSkyLocation();
}

void ACesiumSunSky::_spawnSkySphere() {
  if (!EnableMobileRendering || !IsValid(GetWorld())) {
    return;
  }

  if (!Georeference) {
    return;
  }

  // Set sky sphere actor position to ECEF 0,0,0
  FTransform spawnTransform = FTransform(
      Georeference->InaccurateTransformEcefToUnreal(FVector::ZeroVector));
  SkySphereActor =
      GetWorld()->SpawnActor<AActor>(SkySphereClass, spawnTransform);
  _wantsSpawnMobileSkySphere = false;

  _setSkySphereDirectionalLight();
}

void ACesiumSunSky::UpdateSkySphere() {
  if (!EnableMobileRendering || !SkySphereActor) {
    return;
  }
  UFunction* UpdateSkySphere =
      this->SkySphereActor->FindFunction(TEXT("RefreshMaterial"));
  if (UpdateSkySphere) {
    this->SkySphereActor->ProcessEvent(UpdateSkySphere, NULL);
  }
}

void ACesiumSunSky::_setSkyAtmosphereVisibility(bool bVisible) {
  if (IsValid(SkyLight)) {
    SkyLight->SetVisibility(bVisible);
  }
  if (IsValid(SkyAtmosphereComponent)) {
    SkyAtmosphereComponent->SetVisibility(bVisible);
  }
}

void ACesiumSunSky::_setSkySphereDirectionalLight() {
  if (!EnableMobileRendering || !SkySphereClass || !IsValid(SkySphereActor)) {
    return;
  }

  for (TFieldIterator<FProperty> PropertyIterator(SkySphereClass);
       PropertyIterator;
       ++PropertyIterator) {
    FProperty* prop = *PropertyIterator;
    FName const propName = prop->GetFName();
    if (propName == TEXT("DirectionalLightComponent")) {
      FObjectProperty* objectProp = CastField<FObjectProperty>(prop);
      if (objectProp) {
        UDirectionalLightComponent* directionalLightComponent = nullptr;
        if (UseLevelDirectionalLight) {
// Getting the component from a DirectionalLight actor is editor-only
#if WITH_EDITORONLY_DATA
          directionalLightComponent = LevelDirectionalLight->GetComponent();
#endif
        } else {
          directionalLightComponent = DirectionalLight;
        }
        objectProp->SetPropertyValue_InContainer(
            this->SkySphereActor,
            directionalLightComponent);
      }
    }
  }
}

#if WITH_EDITOR
void ACesiumSunSky::PreEditChange(FProperty* PropertyAboutToChange) {
  Super::PreEditChange(PropertyAboutToChange);

  FName propertyName = PropertyAboutToChange->GetFName();
  if (propertyName == GET_MEMBER_NAME_CHECKED(ACesiumSunSky, Georeference)) {
    if (IsValid(this->Georeference)) {
      this->Georeference->OnGeoreferenceUpdated.RemoveAll(this);
    }
  }
}

void ACesiumSunSky::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {

  const FName propertyName = (PropertyChangedEvent.Property)
                                 ? PropertyChangedEvent.Property->GetFName()
                                 : NAME_None;
  if (propertyName == GET_MEMBER_NAME_CHECKED(ACesiumSunSky, Georeference)) {
    if (IsValid(this->Georeference)) {
      this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
          this,
          &ACesiumSunSky::HandleGeoreferenceUpdated);
      this->HandleGeoreferenceUpdated();
    }
  }
  if (propertyName == GET_MEMBER_NAME_CHECKED(ACesiumSunSky, SkySphereClass)) {
    _wantsSpawnMobileSkySphere = true;
    if (SkySphereActor) {
      SkySphereActor->Destroy();
    }
  }
  if (propertyName ==
      GET_MEMBER_NAME_CHECKED(ACesiumSunSky, EnableMobileRendering)) {
    _wantsSpawnMobileSkySphere = EnableMobileRendering;
    _setSkyAtmosphereVisibility(!EnableMobileRendering);
    if (!EnableMobileRendering && SkySphereActor) {
      SkySphereActor->Destroy();
    }
  }
  if (propertyName ==
          GET_MEMBER_NAME_CHECKED(ACesiumSunSky, UseLevelDirectionalLight) ||
      propertyName ==
          GET_MEMBER_NAME_CHECKED(ACesiumSunSky, LevelDirectionalLight)) {
    _setSkySphereDirectionalLight();
    if (IsValid(LevelDirectionalLight)) {
      LevelDirectionalLight->GetComponent()->SetAtmosphereSunLight(true);
      LevelDirectionalLight->GetComponent()->MarkRenderStateDirty();
    }
  }
  // Place superclass method after variables are updated, so that a new sky
  // sphere can be spawned if needed
  Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

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

  // Convert hours + minutes so far to seconds, and subtract from InSolarTime.
  Second = FMath::RoundToInt((InSolarTime - Hour - Minute / 60) * 3600) % 60;
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
  FDateTime dstStart =
      FDateTime(Year, InDSTStartMonth, InDSTStartDay, InDSTSwitchHour);
  FDateTime dstEnd =
      FDateTime(Year, InDSTEndMonth, InDSTEndDay, InDSTSwitchHour);
  return current >= dstStart && current <= dstEnd;
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
