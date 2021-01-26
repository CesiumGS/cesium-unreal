
#include <string>
#include "CesiumCreditSystem.h"
#include "Cesium3DTiles/CreditSystem.h"

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

    std::string creditString = "<body>\n";
    for (Cesium3DTiles::Credit credit : this->_pCreditSystem->getCreditsToShowThisFrame()) {
        creditString += this->_pCreditSystem->getHtml(credit) + "\n";
    }
    creditString += "</body>";
    Credits = creditString.c_str();
    _pCreditSystem->startNextFrame();
}

void ACesiumCreditSystem::OnConstruction(const FTransform& Transform) {}