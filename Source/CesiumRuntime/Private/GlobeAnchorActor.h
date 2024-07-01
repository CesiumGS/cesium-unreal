// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GlobeAnchorActor.generated.h"

UCLASS(
    DisplayName = "Globe Anchor Actor",
    ClassGroup = (Cesium),
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API AGlobeAnchorActor : public AActor {
  GENERATED_BODY()

public:
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UCesiumGlobeAnchorComponent> GlobeAnchor;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<UChildActorComponent> MoveNode;

  AGlobeAnchorActor();

  virtual void Tick(float DeltaTime) override;

  void SetLocationAndSnap(const FVector& Location);

protected:
  virtual void BeginPlay() override;
};
