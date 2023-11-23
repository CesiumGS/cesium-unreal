// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumIonServer.h"
#include "CesiumRuntimeSettings.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#endif

/*static*/ UCesiumIonServer* UCesiumIonServer::GetOrCreateDefault() {
  UPackage* Package = CreatePackage(
      TEXT("/Game/CesiumSettings/CesiumIonServers/CesiumIonSaaS"));
  Package->FullyLoad();
  UCesiumIonServer* Server =
      Cast<UCesiumIonServer>(Package->FindAssetInPackage());

#if WITH_EDITOR
  if (!IsValid(Server)) {
    UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
    Server = Cast<UCesiumIonServer>(Factory->FactoryCreateNew(
        UCesiumIonServer::StaticClass(),
        Package,
        "CesiumIonSaaS",
        RF_Public | RF_Standalone | RF_Transactional,
        nullptr,
        GWarn));

    Server->DisplayName = TEXT("ion.cesium.com");
    Server->ServerUrl = TEXT("https://ion.cesium.com");
    Server->ApiUrl = TEXT("https://api.cesium.com");
    Server->OAuth2ApplicationID = 190;

    FAssetRegistryModule::AssetCreated(Server);

    Package->FullyLoad();
    Package->SetDirtyFlag(true);
    UEditorLoadingAndSavingUtils::SavePackages({Package}, true);
  }
#endif

  return Server;
}
