
#include "CesiumCreditSystem.h"
#include "Cesium3DTiles/CreditSystem.h"

/*static*/ ACesiumCreditSystem* ACesiumCreditSystem::GetDefaultForActor(AActor* Actor) {
    ACesiumCreditSystem* pCreditSystem = FindObject<ACesiumCreditSystem>(Actor->GetLevel(), TEXT("CesiumCreditSystemDefault"));
	if (!pCreditSystem) {
		FActorSpawnParameters spawnParameters;
		spawnParameters.Name = TEXT("CesiumCreditSystemDefault");
		spawnParameters.OverrideLevel = Actor->GetLevel();
		pCreditSystem = Actor->GetWorld()->SpawnActor<ACesiumCreditSystem>(spawnParameters);
	}
	return pCreditSystem;
}

ACesiumCreditSystem::ACesiumCreditSystem() {
    _pCreditSystem = std::make_shared<Cesium3DTiles::CreditSystem>();
}

ACesiumCreditSystem::~ACesiumCreditSystem() {
    _pCreditSystem.reset();
}

void ACesiumCreditSystem::Tick(float DeltaTime) {
    // placeholder until we can create screen html elements
    std::string creditString = "<body>";
    for (Cesium3DTiles::Credit credit : this->_pCreditSystem->getCreditsToShowThisFrame()) {
        creditString += this->_pCreditSystem->getHtml(credit) + "\n";
    }
    creditString += "</body>";
    Credits = creditString.c_str();
    _pCreditSystem->startNextFrame();
}

void ACesiumCreditSystem::BeginPlay() {}

void ACesiumCreditSystem::OnConstruction(const FTransform& Transform) {}