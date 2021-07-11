// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCreditSystem.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "CesiumCreditSystemBPLoader.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"
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

/*static*/ ACesiumCreditSystem*
ACesiumCreditSystem::GetDefaultForActor(AActor* Actor) {
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

  ACesiumCreditSystem* pACreditSystem =
      findValidDefaultCreditSystem(Actor->GetLevel());
  if (pACreditSystem) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Using existing CreditSystem %s for actor %s"),
        *(pACreditSystem->GetName()),
        *(Actor->GetName()));
    return pACreditSystem;
  }

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Creating default CreditSystem for actor %s"),
      *(Actor->GetName()));

  if (!CesiumCreditSystemBP) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Blueprint not found, unable to retrieve default ACesiumCreditSystem"));
    return nullptr;
  }

  // Make sure that the instance is created in the same
  // level as the actor, and its name starts with the
  // prefix indicating that this is the default instance
  FActorSpawnParameters spawnParameters;
  spawnParameters.Name = TEXT("CesiumCreditSystemDefault");
  spawnParameters.OverrideLevel = Actor->GetLevel();
  spawnParameters.NameMode =
      FActorSpawnParameters::ESpawnActorNameMode::Requested;
  pACreditSystem = Actor->GetWorld()->SpawnActor<ACesiumCreditSystem>(
      CesiumCreditSystemBP,
      spawnParameters);
  return pACreditSystem;
}

ACesiumCreditSystem::ACesiumCreditSystem()
    : _pCreditSystem(std::make_shared<Cesium3DTiles::CreditSystem>()),
      _lastCreditsCount(0) {
  PrimaryActorTick.bCanEverTick = true;
}

bool ACesiumCreditSystem::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumCreditSystem::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  if (!_pCreditSystem) {
    return;
  }

  const std::vector<Cesium3DTiles::Credit>& creditsToShowThisFrame =
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
