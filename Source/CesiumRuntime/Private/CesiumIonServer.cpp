// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumIonServer.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "CesiumIonClient/Connection.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#endif

const wchar_t* DISPLAY_NAME = TEXT("ion.cesium.com");
const wchar_t* SERVER_URL = TEXT("https://ion.cesium.com");
const wchar_t* API_URL = TEXT("https://api.cesium.com");
const int64 OAUTH_APP_ID = 190;

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

    Server->DisplayName = DISPLAY_NAME;
    Server->ServerUrl = SERVER_URL;
    Server->ApiUrl = API_URL;
    Server->OAuth2ApplicationID = OAUTH_APP_ID;

    FAssetRegistryModule::AssetCreated(Server);

    Package->FullyLoad();
    Package->SetDirtyFlag(true);
    UEditorLoadingAndSavingUtils::SavePackages({Package}, true);
  }
#else
  if (!IsValid(Server)) {
    Server = NewObject<UCesiumIonServer>(
        UCesiumIonServer::StaticClass(),
        "CesiumIonSaaS",
        RF_Public | RF_Standalone | RF_Transactional);

    Server->DisplayName = DISPLAY_NAME;
    Server->ServerUrl = SERVER_URL;
    Server->ApiUrl = API_URL;
    Server->OAuth2ApplicationID = OAUTH_APP_ID;
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

CesiumAsync::Future<void> UCesiumIonServer::ResolveApiUrl() {
  if (!this->ApiUrl.IsEmpty())
    return getAsyncSystem().createResolvedFuture();

  if (this->ServerUrl.IsEmpty()) {
    // We don't even have a server URL, so use the SaaS defaults.
    this->ServerUrl = TEXT("https://ion.cesium.com/");
    this->ApiUrl = TEXT("https://api.cesium.com/");
    this->Modify();
    UEditorLoadingAndSavingUtils::SavePackages({this->GetPackage()}, true);
    return getAsyncSystem().createResolvedFuture();
  }

  TObjectPtr<UCesiumIonServer> pServer = this;

  return CesiumIonClient::Connection::getApiUrl(
             getAsyncSystem(),
             getAssetAccessor(),
             TCHAR_TO_UTF8(*this->ServerUrl))
      .thenInMainThread([pServer](std::optional<std::string>&& apiUrl) {
        if (pServer && pServer->ApiUrl.IsEmpty()) {
          pServer->ApiUrl = UTF8_TO_TCHAR(apiUrl->c_str());
          pServer->Modify();
          UEditorLoadingAndSavingUtils::SavePackages(
              {pServer->GetPackage()},
              true);
        }
      });
}
#endif
