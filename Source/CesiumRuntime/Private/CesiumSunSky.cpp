// Fill out your copyright notice in the Description page of Project Settings.

#include "CesiumSunSky.h"
#include "CesiumRuntime.h"

// Sets default values
ACesiumSunSky::ACesiumSunSky() { PrimaryActorTick.bCanEverTick = false; }

#if WITH_EDITOR
void ACesiumSunSky::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (!PropertyChangedEvent.Property) {
    return;
  }

  FName PropName = PropertyChangedEvent.Property->GetFName();
  if (PropName == GET_MEMBER_NAME_CHECKED(ACesiumSunSky, Georeference)) {
    if (IsValid(this->Georeference)) {
      this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
          this,
          &ACesiumSunSky::HandleGeoreferenceUpdated);
      this->HandleGeoreferenceUpdated();
    }
  }
}
#endif

void ACesiumSunSky::PostInitProperties() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostInitProperties on actor %s"),
      *this->GetName());

  Super::PostInitProperties();

  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }
  if (IsValid(this->Georeference)) {
    this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
        this,
        &ACesiumSunSky::HandleGeoreferenceUpdated);
    this->HandleGeoreferenceUpdated();
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
