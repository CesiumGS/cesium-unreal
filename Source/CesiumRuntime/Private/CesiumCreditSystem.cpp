// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCreditSystem.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "CesiumCreditSystemBPLoader.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include <string>
#include <vector>

/*static*/ UClass* ACesiumCreditSystem::CesiumCreditSystemBP = nullptr;

namespace {

/**
 * @brief Tries to find the default credit system in the given level.
 *
 * This will search all actors of the given level for a `ACesiumCreditSystem`
 * whose name starts with `"CesiumCreditSystemDefault"` that is *valid*
 * (i.e. not pending kill).
 *
 * @param Level The level
 * @return The default credit system, or `nullptr` if there is none.
 */
ACesiumCreditSystem* findValidDefaultCreditSystem(ULevel* Level) {
  if (!IsValid(Level)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("No valid level for findValidDefaultCreditSystem"));
    return nullptr;
  }
  TArray<AActor*>& Actors = Level->Actors;
  AActor** DefaultCreditSystemPtr =
      Actors.FindByPredicate([](AActor* const& InItem) {
        if (!IsValid(InItem)) {
          return false;
        }
        if (!InItem->IsA(ACesiumCreditSystem::StaticClass())) {
          return false;
        }
        if (!InItem->GetName().StartsWith("CesiumCreditSystemDefault")) {
          return false;
        }
        return true;
      });
  if (!DefaultCreditSystemPtr) {
    return nullptr;
  }
  AActor* DefaultCreditSystem = *DefaultCreditSystemPtr;
  return Cast<ACesiumCreditSystem>(DefaultCreditSystem);
}
} // namespace

FName ACesiumCreditSystem::DEFAULT_CREDITSYSTEM_TAG =
    FName("DEFAULT_CREDITSYSTEM");

/*static*/ ACesiumCreditSystem*
ACesiumCreditSystem::GetDefaultCreditSystem(const UObject* WorldContextObject) {
  // Blueprint loading can only happen in a constructor, so we instantiate a
  // loader object that retrieves the blueprint class in its constructor. We can
  // destroy the loader immediately once it's done since it will have already
  // set CesiumCreditSystemBP.
  if (!CesiumCreditSystemBP) {
    UCesiumCreditSystemBPLoader* bpLoader =
        NewObject<UCesiumCreditSystemBPLoader>();
    CesiumCreditSystemBP = bpLoader->CesiumCreditSystemBP;
    bpLoader->ConditionalBeginDestroy();
  }

  UWorld* world = WorldContextObject->GetWorld();
  // This method can be called by actors even when opening the content browser.
  if (!IsValid(world)) {
    return nullptr;
  }
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("World name for GetDefaultCreditSystem: %s"),
      *world->GetFullName());

  // Note: The actor iterator will be created with the
  // "EActorIteratorFlags::SkipPendingKill" flag,
  // meaning that we don't have to handle objects
  // that have been deleted. (This is the default,
  // but made explicit here)
  ACesiumCreditSystem* pCreditSystem = nullptr;
  EActorIteratorFlags flags = EActorIteratorFlags::OnlyActiveLevels |
                              EActorIteratorFlags::SkipPendingKill;
  for (TActorIterator<AActor> actorIterator(
           world,
           ACesiumCreditSystem::StaticClass(),
           flags);
       actorIterator;
       ++actorIterator) {
    AActor* actor = *actorIterator;
    if (actor->ActorHasTag(DEFAULT_CREDITSYSTEM_TAG)) {
      pCreditSystem = Cast<ACesiumCreditSystem>(actor);
      break;
    }
  }
  if (!pCreditSystem) {
    // Legacy method of finding Georeference, for backwards compatibility with
    // existing projects
    ACesiumCreditSystem* pCreditSystemCandidate =
        findValidDefaultCreditSystem(world->PersistentLevel);

    // Test if PendingKill
    if (IsValid(pCreditSystemCandidate)) {
      pCreditSystem = pCreditSystemCandidate;
    }
  }
  if (!pCreditSystem) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Creating default Credit System for actor %s"),
        *WorldContextObject->GetName());
    // Spawn georeference in the persistent level
    FActorSpawnParameters spawnParameters;
    spawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    pCreditSystem = world->SpawnActor<ACesiumCreditSystem>(
        CesiumCreditSystemBP,
        spawnParameters);
    // Null check so the editor doesn't crash when it makes arbitrary calls to
    // this function without a valid world context object.
    if (pCreditSystem) {
      pCreditSystem->Tags.Add(DEFAULT_CREDITSYSTEM_TAG);
    }
  } else {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Using existing CreditSystem %s for actor %s"),
        *pCreditSystem->GetName(),
        *WorldContextObject->GetName());
  }
  return pCreditSystem;
}

ACesiumCreditSystem::ACesiumCreditSystem()
    : _pCreditSystem(std::make_shared<Cesium3DTilesSelection::CreditSystem>()),
      _lastCreditsCount(0) {
  PrimaryActorTick.bCanEverTick = true;
}

bool ACesiumCreditSystem::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumCreditSystem::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  if (!_pCreditSystem) {
    return;
  }

  const std::vector<Cesium3DTilesSelection::Credit>& creditsToShowThisFrame =
      _pCreditSystem->getCreditsToShowThisFrame();

  // if the credit list has changed, we want to reformat the credits
  CreditsUpdated =
      creditsToShowThisFrame.size() != _lastCreditsCount ||
      _pCreditSystem->getCreditsToNoLongerShowThisFrame().size() > 0;
  if (CreditsUpdated) {
    std::string creditString =
        "<head>\n<meta charset=\"utf-16\"/>\n</head>\n<body style=\"color:white\"><ul>";
    for (size_t i = 0; i < creditsToShowThisFrame.size(); ++i) {
      creditString +=
          "<li>" + _pCreditSystem->getHtml(creditsToShowThisFrame[i]) + "</li>";
    }
    creditString += "</ul></body>";
    Credits = UTF8_TO_TCHAR(creditString.c_str());

    _lastCreditsCount = creditsToShowThisFrame.size();
  }

  _pCreditSystem->startNextFrame();
}
