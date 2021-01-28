
#include "CesiumCreditSystem.h"
#include "UnrealConversions.h"
#include "Cesium3DTiles/CreditSystem.h"
#include <string>
#include <vector>

/*static*/ ACesiumCreditSystem* ACesiumCreditSystem::GetDefaultForActor(AActor* Actor) {
    ACesiumCreditSystem* pACreditSystem = FindObject<ACesiumCreditSystem>(Actor->GetLevel(), TEXT("CesiumCreditSystemDefault"));
    if (!pACreditSystem) {
        FActorSpawnParameters spawnParameters;
        spawnParameters.Name = TEXT("CesiumCreditSystemDefault");
        spawnParameters.OverrideLevel = Actor->GetLevel();
        pACreditSystem = Actor->GetWorld()->SpawnActor<ACesiumCreditSystem>(spawnParameters);
    }
    return pACreditSystem;
}

ACesiumCreditSystem::ACesiumCreditSystem() {
    PrimaryActorTick.bCanEverTick = true;
    _pCreditSystem = std::make_shared<Cesium3DTiles::CreditSystem>();
}

ACesiumCreditSystem::~ACesiumCreditSystem() {
    _pCreditSystem.reset();
}

bool ACesiumCreditSystem::ShouldTickIfViewportsOnly() const {
    return true;
}

void ACesiumCreditSystem::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    std::vector<Cesium3DTiles::Credit> creditsToShowThisFrame = _pCreditSystem->getCreditsToShowThisFrame();

    // if the credit list has changed, we want to reformat the credits
    if (creditsToShowThisFrame.size() != _lastCreditsCount || _pCreditSystem->getCreditsToNoLongerShowThisFrame().size() > 0) {
        std::string creditString = "<head>\n<meta charset=\"utf-16\"/>\n</head>\n<body style=\"color:white\"><ul>";
        for (size_t i = 0; i < creditsToShowThisFrame.size(); ++i) {
            creditString += "<li>" + _pCreditSystem->getHtml(creditsToShowThisFrame[i]) + "</li>";
        } 
        creditString += "</ul></body>";
        Credits = utf8_to_wstr(creditString);
        
        _lastCreditsCount = creditsToShowThisFrame.size();
    }

    _pCreditSystem->startNextFrame();
}

void ACesiumCreditSystem::OnConstruction(const FTransform& Transform) {}