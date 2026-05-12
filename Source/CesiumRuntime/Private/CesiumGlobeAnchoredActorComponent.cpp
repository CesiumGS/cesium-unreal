// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGlobeAnchoredActorComponent.h"
#include "CesiumGlobeAnchorComponent.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

UCesiumGlobeAnchorComponent*
UCesiumGlobeAnchoredActorComponent::GetGlobeAnchor() {
  return this->GlobeAnchor;
}

void UCesiumGlobeAnchoredActorComponent::OnRegister() {
  Super::OnRegister();
  this->ResolveGlobeAnchor();
}

void UCesiumGlobeAnchoredActorComponent::BeginPlay() {
  Super::BeginPlay();
  this->ResolveGlobeAnchor();
}

void UCesiumGlobeAnchoredActorComponent::ResolveGlobeAnchor() {
  this->GlobeAnchor = nullptr;

  AActor* Owner = this->GetOwner();
  if (!IsValid(Owner))
    return;

  this->GlobeAnchor =
      Owner->FindComponentByClass<UCesiumGlobeAnchorComponent>();
  if (!IsValid(this->GlobeAnchor)) {
    // A globe anchor is missing and required, so add one.
    this->GlobeAnchor =
        Cast<UCesiumGlobeAnchorComponent>(Owner->AddComponentByClass(
            UCesiumGlobeAnchorComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
    Owner->AddInstanceComponent(this->GlobeAnchor);

    // Force the Editor to refresh to show the newly-added component
#if WITH_EDITOR
    Owner->Modify();
    if (Owner->IsSelectedInEditor()) {
      GEditor->SelectActor(Owner, true, true, true, true);
    }
#endif
  }
}
