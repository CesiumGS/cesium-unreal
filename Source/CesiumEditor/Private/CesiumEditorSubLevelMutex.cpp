// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumEditorSubLevelMutex.h"
#include "Async/Async.h"
#include "CesiumGeoreference.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelSwitcherComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
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

  bool needsTick = false;

  if (!pLevelInstance->IsTemporarilyHiddenInEditor(true)) {
    pSwitcher->SetTargetSubLevel(pLevelInstance);
    needsTick = true;
  } else if (pSwitcher->GetTargetSubLevel() == pLevelInstance) {
    pSwitcher->SetTargetSubLevel(nullptr);
    needsTick = true;
  }

  UWorld* pWorld = pGeoreference->GetWorld();
  if (needsTick && pWorld && !pWorld->IsGameWorld()) {
    // Other sub-levels won't be deactivated until
    // UCesiumSubLevelSwitcherComponent next ticks. Normally that's no problem,
    // but in some unusual cases it will never happen. For example, in UE 5.3,
    // when running tests on CI with `-nullrhi`. Or if you close all your
    // viewports in the Editor. So here we schedule a game thread task to ensure
    // that _updateSubLevelStateEditor is called. It won't do any harm if we are
    // ticking and it ends up being called multiple times.
    TWeakObjectPtr<UCesiumSubLevelSwitcherComponent> pSwitcherWeak = pSwitcher;
    AsyncTask(ENamedThreads::GameThread, [pSwitcherWeak]() {
      UCesiumSubLevelSwitcherComponent* pSwitcher = pSwitcherWeak.Get();
      if (pSwitcher) {
        pSwitcher->_updateSubLevelStateEditor();
      }
    });
  }
}
