// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumOriginShiftComponent.generated.h"

class UCesiumGlobeAnchorComponent;

UCLASS(ClassGroup = "Cesium", Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumOriginShiftComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UCesiumOriginShiftComponent();

protected:
  virtual void OnRegister() override;
  virtual void BeginPlay() override;
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

private:
  void ResolveGlobeAnchor();

  // The globe anchor attached to the same Actor as this component. Don't
  // save/load or copy this. It is set in BeginPlay.
  UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
  UCesiumGlobeAnchorComponent* GlobeAnchor;
};
