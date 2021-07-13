// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include <CesiumGeoreferenceComponent.h>

#include "CesiumTestClasses.generated.h"

/**
 * A trivial actor for tests, that has a GeoreferenceComponent
 */
UCLASS()
class ACesiumGeoreferenceComponentTestActor : public AActor {
  GENERATED_BODY()
public:
  ACesiumGeoreferenceComponentTestActor() {

    RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

    _georeferenceComponent =
        CreateDefaultSubobject<UCesiumGeoreferenceComponent>(
            TEXT("Georeference"));

    // NOTE: This call will no longer be necessary after the
    // Georeference refactoring, because the GeoreferenceComponent
    // then is only an actor component, and no scene component
    _georeferenceComponent->SetupAttachment(RootComponent);
  }

  UPROPERTY(VisibleAnywhere)
  UCesiumGeoreferenceComponent* _georeferenceComponent;
};
