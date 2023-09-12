// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumEditorSubLevelMutex.h"
#include "CesiumGeoreference.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "Components/ActorComponent.h"
#include "LevelInstance/LevelInstanceActor.h"

CesiumEditorSubLevelMutex::CesiumEditorSubLevelMutex() {
  this->_subscription = UActorComponent::MarkRenderStateDirtyEvent.AddRaw(
      this,
      &CesiumEditorSubLevelMutex::OnMarkRenderStateDirty);
}

CesiumEditorSubLevelMutex::~CesiumEditorSubLevelMutex() {
  UActorComponent::MarkRenderStateDirtyEvent.Remove(this->_subscription);
  this->_subscription.Reset();
}

void CesiumEditorSubLevelMutex::OnMarkRenderStateDirty(
    UActorComponent& component) {
  UCesiumSubLevelComponent* pSubLevel =
      Cast<UCesiumSubLevelComponent>(&component);
  if (pSubLevel == nullptr)
    return;

  ALevelInstance* pLevelInstance = Cast<ALevelInstance>(pSubLevel->GetOwner());
  if (pLevelInstance == nullptr)
    return;

  ACesiumGeoreference* pGeoreference = pSubLevel->GetResolvedGeoreference();
  if (pGeoreference == nullptr)
    return;

  UCesiumSubLevelSwitcherComponent* pSwitcher =
      pGeoreference->FindComponentByClass<UCesiumSubLevelSwitcherComponent>();
  if (pSwitcher == nullptr)
    return;

  if (!pLevelInstance->IsTemporarilyHiddenInEditor(true)) {
    pSwitcher->SetTargetSubLevel(pLevelInstance);
  } else if (pSwitcher->GetTargetSubLevel() == pLevelInstance) {
    pSwitcher->SetTargetSubLevel(nullptr);
  }
}
