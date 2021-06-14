// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumSunSky.h"


// Sets default values
ACesiumSunSky::ACesiumSunSky() {
  // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;

  if (!Georeference) {
    Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  if (Georeference) {
    Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
        this,
        &ACesiumSunSky::HandleGeoreferenceUpdated);
  }

}

// Called when the game starts or when spawned
void ACesiumSunSky::BeginPlay() {
  Super::BeginPlay();

}

// Called every frame
void ACesiumSunSky::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

}
