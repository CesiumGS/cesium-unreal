// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumSunSky.h"
#include "CesiumCustomVersion.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumRuntime.h"
#include "Kismet/GameplayStatics.h"
#include "SunPosition.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "VecMath.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#endif

// The UE SkyAtmosphere assumes Earth is a sphere. But it's closer to an oblate
// spheroid, where the radius at the poles is ~21km less than the radius at the
// equator. And on top of that, there's terrain, causing bumps of up to 8km or
// so (Mount Everest). Mean Sea Level is nowhere more than 100 meters different
// from the WGS84 ellipsoid, and the lowest dry land point on Earth is the Dead
// Sea at about 432 meters below sea level. So all up, the worst case "ground
// radius" for atmosphere purposes ranges from about 6356km to about 6387km
// depending on where you are on Earth. That's a range of 31km, which definitely
// matters. We can't pick a single globe radius that will work everywhere on
// Earth.
//
// So, our strategy here is:
//  * When close to the surface, it's important that the radius not be too
//  large, or else there will be a gap between the bottom of the atmosphere and
//  the top of the terrain. To avoid that, we want to use a tight fitting globe
//  radius that approximates mean sea level at the camera's position and is
//  guaranteed to be below it. Rather than actually calculate sea level, a WGS84
//  height of -100meters will be close enough.
//  * When far from the surface, we can see a lot of the Earth, and it's
//  essential that no bits of the surface extend into the atmosphere, because
//  that creates a very distracting artifact. So we want to choose a globe
//  radius that is guaranteed to encapsulate all visible parts of the globe.
//  * In between these two extremes, we need to blend smoothly.

// Sets default values
ACesiumSunSky::ACesiumSunSky() {
  PrimaryActorTick.bCanEverTick = true;

  Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
  SetRootComponent(Scene);

  DirectionalLight = CreateDefaultSubobject<UDirectionalLightComponent>(
      TEXT("DirectionalLight"));
  DirectionalLight->SetupAttachment(Scene);
  DirectionalLight->SetWorldLocation(FVector(0, 0, 0));
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
  SkyLight->SetWorldLocation(FVector(0, 0, 150));
  SkyLight->SetMobility(EComponentMobility::Movable);
  SkyLight->bRealTimeCapture = true;
  SkyLight->bLowerHemisphereIsBlack = false;
  SkyLight->bTransmission = true;
  SkyLight->bCastRaytracedShadow = true;
  SkyLight->SamplesPerPixel = 2;

  SkyAtmosphere =
      CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
  SkyAtmosphere->SetupAttachment(Scene);
  SkyAtmosphere->TransformMode =
      ESkyAtmosphereTransformMode::PlanetCenterAtComponentTransform;

  this->GlobeAnchor =
      CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
  this->GlobeAnchor->AdjustOrientationForGlobeWhenMoving = false;
  this->GlobeAnchor->MoveToECEF(glm::dvec3(0.0, 0.0, 0.0));
}

void ACesiumSunSky::_handleTransformUpdated(
    USceneComponent* InRootComponent,
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport) {
  // This Actor generally shouldn't move with respect to the globe, but this
  // method will be called on georeference change. We need to update the sun
  // position for the new UE coordinate system.
  this->UpdateSun();
}

void ACesiumSunSky::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction(Transform);

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnConstruction for CesiumSunSky %s"),
      *this->GetName());

  this->GlobeAnchor->MoveToECEF(glm::dvec3(0.0, 0.0, 0.0));

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Spawn new sky sphere: %s"),
      _wantsSpawnMobileSkySphere ? TEXT("true") : TEXT("false"));
  if (EnableMobileRendering) {
    DirectionalLight->Intensity = MobileDirectionalLightIntensity;
    if (_wantsSpawnMobileSkySphere && SkySphereClass) {
      _spawnSkySphere();
    }
  }
  _setSkyAtmosphereVisibility(!EnableMobileRendering);

  this->UpdateSun();
}

void ACesiumSunSky::_spawnSkySphere() {
  if (!EnableMobileRendering || !IsValid(GetWorld())) {
    return;
  }

  if (!IsValid(this->GetGeoreference())) {
    return;
  }

  // Create a new Sky Sphere Actor and anchor it to the center of the Earth.
  this->SkySphereActor = GetWorld()->SpawnActor<AActor>(SkySphereClass);

  // Anchor it to the center of the Earth.
  UCesiumGlobeAnchorComponent* GlobeAnchorComponent =
      NewObject<UCesiumGlobeAnchorComponent>(SkySphereActor);
  this->SkySphereActor->AddInstanceComponent(GlobeAnchorComponent);
  GlobeAnchorComponent->AdjustOrientationForGlobeWhenMoving = false;
  GlobeAnchorComponent->SetGeoreference(this->GlobeAnchor->GetGeoreference());
  GlobeAnchorComponent->MoveToECEF(glm::dvec3(0.0, 0.0, 0.0));

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

void ACesiumSunSky::BeginPlay() {
  Super::BeginPlay();

  this->GlobeAnchor->MoveToECEF(glm::dvec3(0.0, 0.0, 0.0));

  this->_transformUpdatedSubscription =
      this->RootComponent->TransformUpdated.AddUObject(
          this,
          &ACesiumSunSky::_handleTransformUpdated);
}

void ACesiumSunSky::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (this->_transformUpdatedSubscription.IsValid()) {
    this->RootComponent->TransformUpdated.Remove(
        this->_transformUpdatedSubscription);
    this->_transformUpdatedSubscription.Reset();
  }
}

void ACesiumSunSky::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  Ar.UsingCustomVersion(FCesiumCustomVersion::GUID);

  const int32 CesiumVersion = Ar.CustomVer(FCesiumCustomVersion::GUID);

  if (Ar.IsLoading() &&
      CesiumVersion < FCesiumCustomVersion::GeoreferenceRefactoring) {
    // Now that CesiumSunSky is a C++ class, its Components should be marked
    // with a CreationMethod of Native, and they are to start. But because
    // CesiumSunSky was a Blueprints class in old versions, the CreationMethod
    // of its components gets set to SimpleConstructionScript on load, which
    // causes those components to later (and erroneously) be removed. So we
    // reset the creation method here.
    Ar.Preload(this->RootComponent);
    this->RootComponent->CreationMethod = EComponentCreationMethod::Native;
    Ar.Preload(this->SkyLight);
    this->SkyLight->CreationMethod = EComponentCreationMethod::Native;
    Ar.Preload(this->DirectionalLight);
    this->DirectionalLight->CreationMethod = EComponentCreationMethod::Native;
    Ar.Preload(this->SkyAtmosphere);
    this->SkyAtmosphere->CreationMethod = EComponentCreationMethod::Native;
  }
}

void ACesiumSunSky::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);

  if (this->UpdateAtmosphereAtRuntime) {
    this->AdjustAtmosphereRadius();
  }
}

void ACesiumSunSky::PostLoad() {
  Super::PostLoad();

  // For backward compatibility, copy the value of the deprecated Georeference
  // property to its new home in the GlobeAnchor. It doesn't appear to be
  // possible to do this in Serialize:
  // https://udn.unrealengine.com/s/question/0D54z00007CAbHFCA1/backward-compatibile-serialization-for-uobject-pointers
  const int32 CesiumVersion =
      this->GetLinkerCustomVersion(FCesiumCustomVersion::GUID);
  if (CesiumVersion < FCesiumCustomVersion::GeoreferenceRefactoring) {
    if (this->Georeference_DEPRECATED != nullptr && this->GlobeAnchor &&
        this->GlobeAnchor->GetGeoreference() == nullptr) {
      this->GlobeAnchor->SetGeoreference(this->Georeference_DEPRECATED);
    }
  }
}

bool ACesiumSunSky::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumSunSky::_setSkyAtmosphereVisibility(bool bVisible) {
  if (IsValid(SkyLight)) {
    SkyLight->SetVisibility(bVisible);
  }
  if (IsValid(SkyAtmosphere)) {
    SkyAtmosphere->SetVisibility(bVisible);
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
void ACesiumSunSky::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {

  const FName propertyName = (PropertyChangedEvent.Property)
                                 ? PropertyChangedEvent.Property->GetFName()
                                 : NAME_None;
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

ACesiumGeoreference* ACesiumSunSky::GetGeoreference() const {
  if (!IsValid(this->GlobeAnchor)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("ACesiumSunSky %s does not have a GlobeAnchorComponent"),
        *this->GetName());
    return nullptr;
  }
  return this->GlobeAnchor->ResolveGeoreference();
}

void ACesiumSunSky::UpdateSun_Implementation() {
  bool isDST = this->IsDST(
      this->UseDaylightSavingTime,
      this->DSTStartMonth,
      this->DSTStartDay,
      this->DSTEndMonth,
      this->DSTEndDay,
      this->DSTSwitchHour);

  int32 hours, minutes, seconds;
  this->GetHMSFromSolarTime(this->SolarTime, hours, minutes, seconds);

  FSunPositionData sunPosition;
  USunPositionFunctionLibrary::GetSunPosition(
      this->GetGeoreference()->OriginLatitude,
      this->GetGeoreference()->OriginLongitude,
      this->TimeZone,
      isDST,
      this->Year,
      this->Month,
      this->Day,
      hours,
      minutes,
      seconds,
      sunPosition);

  this->Elevation = sunPosition.Elevation - 180.0f;
  this->CorrectedElevation = sunPosition.CorrectedElevation - 180.0f;
  this->Azimuth = sunPosition.Azimuth;

  FRotator newRotation(
      -this->CorrectedElevation,
      180.0f + (this->Azimuth + this->NorthOffset),
      0.0f);

  // Orient sun / directional light
  if (this->UseLevelDirectionalLight && IsValid(this->LevelDirectionalLight) &&
      IsValid(this->LevelDirectionalLight->GetRootComponent())) {
    this->LevelDirectionalLight->GetRootComponent()->SetWorldRotation(
        newRotation);
  } else {
    this->DirectionalLight->SetWorldRotation(newRotation);
  }

  // Mobile only
  this->UpdateSkySphere();
}

namespace {

FVector getViewLocation(UWorld* pWorld) {
#if WITH_EDITOR
  if (!pWorld->IsGameWorld()) {
    // Grab the location of the active viewport.
    FViewport* pViewport = GEditor->GetActiveViewport();

    const TArray<FEditorViewportClient*>& viewportClients =
        GEditor->GetAllViewportClients();
    for (FEditorViewportClient* pEditorViewportClient : viewportClients) {
      if (pEditorViewportClient &&
          pEditorViewportClient == pViewport->GetClient()) {
        return pEditorViewportClient->GetViewLocation();
      }
    }
  }
#endif

  // Get the player's current globe location.
  APawn* pPawn = UGameplayStatics::GetPlayerPawn(pWorld, 0);
  if (!pPawn) {
    return pPawn->GetActorLocation();
  }

  return FVector(0.0f, 0.0f, 0.0f);
}

} // namespace

void ACesiumSunSky::AdjustAtmosphereRadius() {
  FVector location = getViewLocation(this->GetWorld());
  glm::dvec3 llh =
      this->GetGeoreference()->TransformUnrealToLongitudeLatitudeHeight(
          VecMath::createVector3D(location));

  // An atmosphere of this radius should circumscribe all Earth terrain.
  double maxRadius = 6387000.0;

  if (llh.z / 1000.0 > this->CircumscribedGroundThreshold) {
    this->SetSkyAtmosphereGroundRadius(this->SkyAtmosphere, maxRadius / 1000.0);
  } else {
    // Find the ellipsoid radius 100m below the surface at this location. See
    // the comment at the top of this file.
    glm::dvec3 ecef = this->GetGeoreference()
                          ->GetGeoTransforms()
                          .TransformLongitudeLatitudeHeightToEcef(
                              glm::dvec3(llh.x, llh.y, -100.0));
    double minRadius = glm::length(ecef);

    if (llh.z / 1000.0 < this->InscribedGroundThreshold) {
      this->SetSkyAtmosphereGroundRadius(
          this->SkyAtmosphere,
          minRadius / 1000.0);
    } else {
      double t =
          ((llh.z / 1000.0) - this->InscribedGroundThreshold) /
          (this->CircumscribedGroundThreshold - this->InscribedGroundThreshold);
      double radius = glm::mix(minRadius, maxRadius, t);
      this->SetSkyAtmosphereGroundRadius(this->SkyAtmosphere, radius / 1000.0);
    }
  }
}

void ACesiumSunSky::GetHMSFromSolarTime(
    float InSolarTime,
    int32& Hour,
    int32& Minute,
    int32& Second) {
  Hour = FMath::TruncToInt(InSolarTime) % 24;
  Minute = FMath::TruncToInt((InSolarTime - Hour) * 60) % 60;

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
