// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumIonServer.h"
#include "CesiumRuntimeSettings.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#endif

/*static*/ UCesiumIonServer* UCesiumIonServer::_pDefaultForNewObjects = nullptr;

/*static*/ UCesiumIonServer* UCesiumIonServer::GetDefaultServer() {
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

/*static*/ UCesiumIonServer* UCesiumIonServer::GetServerForNewObjects() {
  if (IsValid(_pDefaultForNewObjects)) {
    return _pDefaultForNewObjects;
  } else {
    return GetDefaultServer();
  }
}

/*static*/ void
UCesiumIonServer::SetServerForNewObjects(UCesiumIonServer* Server) {
  _pDefaultForNewObjects = Server;
}

#if WITH_EDITOR
UCesiumIonServer*
UCesiumIonServer::GetBackwardCompatibleServer(const FString& apiUrl) {
  // Return the default server if the API URL is unspecified or if it's the
  // standard SaaS API URL.
  if (apiUrl.IsEmpty() ||
      apiUrl.StartsWith(TEXT("https://api.ion.cesium.com")) ||
      apiUrl.StartsWith(TEXT("https://api.cesium.com"))) {
    return UCesiumIonServer::GetDefaultServer();
  }

  // Find a server with this API URL.
  TArray<FAssetData> CesiumIonServers;
  FAssetRegistryModule& AssetRegistryModule =
      FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
  AssetRegistryModule.Get().GetAssetsByClass(
      UCesiumIonServer::StaticClass()->GetClassPathName(),
      CesiumIonServers);

  FAssetData* pFound =
      CesiumIonServers.FindByPredicate([&apiUrl](const FAssetData& asset) {
        UCesiumIonServer* pServer = Cast<UCesiumIonServer>(asset.GetAsset());
        return pServer && pServer->ApiUrl == apiUrl;
      });

  if (pFound) {
    return Cast<UCesiumIonServer>(pFound->GetAsset());
  }

  // Not found - create a new server asset.
  UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();

  UPackage* Package = nullptr;

  FString PackageBasePath = TEXT("/Game/CesiumSettings/CesiumIonServers/");
  FString PackageName;
  FString PackagePath;

  const int ArbitraryPackageIndexLimit = 10000;

  for (int i = 0; i < ArbitraryPackageIndexLimit; ++i) {
    PackageName = TEXT("FromApiUrl") + FString::FromInt(i);
    PackagePath = PackageBasePath + PackageName;
    Package = FindPackage(nullptr, *PackagePath);
    if (Package == nullptr) {
      Package = CreatePackage(*PackagePath);
      break;
    }
  }

  if (Package == nullptr)
    return nullptr;

  Package->FullyLoad();

  UCesiumIonServer* Server = Cast<UCesiumIonServer>(Factory->FactoryCreateNew(
      UCesiumIonServer::StaticClass(),
      Package,
      FName(PackageName),
      RF_Public | RF_Standalone | RF_Transactional,
      nullptr,
      GWarn));

  Server->DisplayName = apiUrl;
  Server->ServerUrl = apiUrl;
  Server->ApiUrl = apiUrl;
  Server->OAuth2ApplicationID = 190;

  // Adopt the token from the default server, consistent with the behavior in
  // old versions of Cesium for Unreal.
  UCesiumIonServer* pDefault = UCesiumIonServer::GetDefaultServer();
  Server->DefaultIonAccessTokenId = pDefault->DefaultIonAccessTokenId;
  Server->DefaultIonAccessToken = pDefault->DefaultIonAccessToken;

  FAssetRegistryModule::AssetCreated(Server);

  Package->FullyLoad();
  Package->SetDirtyFlag(true);
  UEditorLoadingAndSavingUtils::SavePackages({Package}, true);

  return Server;
}
#endif
