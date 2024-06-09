// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "GlobeAnchorActor.h"

#include "CesiumGlobeAnchorComponent.h"

AGlobeAnchorActor::AGlobeAnchorActor() {
  PrimaryActorTick.bCanEverTick = true;

  GlobeAnchor =
      CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
  MoveNode = CreateDefaultSubobject<UChildActorComponent>(TEXT("MoveNode"));
  RootComponent = MoveNode;
}

void AGlobeAnchorActor::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

void AGlobeAnchorActor::SetLocationAndSnap(const FVector& Location) {
  SetActorLocation(Location);
  SetActorScale3D(FVector::One());
  GlobeAnchor->SnapToEastSouthUp();
}

void AGlobeAnchorActor::BeginPlay() { Super::BeginPlay(); }
