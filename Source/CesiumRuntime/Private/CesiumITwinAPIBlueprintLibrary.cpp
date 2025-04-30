#include "CesiumITwinAPIBlueprintLibrary.h"

#include "CesiumITwinCesiumCuratedContentRasterOverlay.h"
#include "CesiumRuntime.h"
#include <CesiumUtility/Result.h>

void UCesiumITwinResource::Spawn() {
  UWorld* pWorld = GetWorld();
  check(pWorld);

  ACesiumGeoreference* pParent =
      ACesiumGeoreference::GetDefaultGeoreference(pWorld);

  check(pParent);

  if (_resource.resourceType == CesiumITwinClient::ResourceType::Imagery) {
    // TODO: don't just attach imagery to the last tileset spawned!
    TArray<AActor*> ChildActors;
    pParent->GetAttachedActors(ChildActors);
    for (auto It = ChildActors.rbegin(); It != ChildActors.rend(); ++It) {
      ACesium3DTileset* pChildTileset = Cast<ACesium3DTileset>(*It);
      if (IsValid(pChildTileset) &&
          pChildTileset->GetTilesetSource() ==
              ETilesetSource::FromITwinCesiumCuratedContent) {

        UCesiumITwinCesiumCuratedContentRasterOverlay* pOverlay =
            NewObject<UCesiumITwinCesiumCuratedContentRasterOverlay>(
                pChildTileset,
                FName(TEXT("Overlay0")),
                RF_Transactional);
        pOverlay->MaterialLayerKey = "Overlay0";
        pOverlay->ITwinAccessToken = UTF8_TO_TCHAR(
            this->_pConnection->getAccessToken().getToken().c_str());
        pOverlay->AssetID = std::stoi(_resource.id);
        pOverlay->SetActive(true);
        pOverlay->OnComponentCreated();
        pChildTileset->AddInstanceComponent(pOverlay);
        break;
      }
    }
    return;
  }

  ACesium3DTileset* pTileset = pWorld->SpawnActor<ACesium3DTileset>();
  pTileset->AttachToActor(
      pParent,
      FAttachmentTransformRules::KeepRelativeTransform);

  pTileset->SetITwinAccessToken(
      UTF8_TO_TCHAR(this->_pConnection->getAccessToken().getToken().c_str()));

  switch (_resource.source) {
  case CesiumITwinClient::ResourceSource::CesiumCuratedContent:
    pTileset->SetTilesetSource(ETilesetSource::FromITwinCesiumCuratedContent);
    pTileset->SetITwinCesiumContentID(std::stoi(_resource.id));
    break;
  case CesiumITwinClient::ResourceSource::MeshExport:
    pTileset->SetTilesetSource(ETilesetSource::FromIModelMeshExportService);
    pTileset->SetIModelID(UTF8_TO_TCHAR(_resource.parentId->c_str()));
    break;
  case CesiumITwinClient::ResourceSource::RealityData:
    pTileset->SetTilesetSource(ETilesetSource::FromITwinRealityData);
    pTileset->SetITwinID(UTF8_TO_TCHAR(_resource.parentId->c_str()));
    pTileset->SetRealityDataID(UTF8_TO_TCHAR(_resource.id.c_str()));
    break;
  }
}

UCesiumITwinAPIAuthorizeAsyncAction*
UCesiumITwinAPIAuthorizeAsyncAction::Authorize(const FString& ClientID) {
  UCesiumITwinAPIAuthorizeAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIAuthorizeAsyncAction>();
  pAsyncAction->_clientId = ClientID;
  return pAsyncAction;
}

void UCesiumITwinAPIAuthorizeAsyncAction::Activate() {
  CesiumITwinClient::Connection::authorize(
      getAsyncSystem(),
      getAssetAccessor(),
      "Cesium for Unreal",
      TCHAR_TO_UTF8(*this->_clientId),
      "/itwin/auth/redirect",
      5081,
      std::vector<std::string>{"itwin-platform", "offline_access"},
      [&Callback = this->OnAuthorizationEvent](const std::string& url) {
        Callback.Broadcast(
            ECesiumITwinDelegateType::OpenUrl,
            UTF8_TO_TCHAR(url.c_str()),
            nullptr,
            TArray<FString>());
      })
      .thenInMainThread([this](CesiumUtility::Result<
                               CesiumITwinClient::Connection>&& connection) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Authorization finished but authorize async action is no longer valid."));
          return;
        }

        if (!connection.value) {
          TArray<FString> Errors;
          for (const std::string& error : connection.errors.errors) {
            Errors.Emplace(UTF8_TO_TCHAR(error.c_str()));
          }

          for (const std::string& warning : connection.errors.warnings) {
            Errors.Emplace(UTF8_TO_TCHAR(warning.c_str()));
          }

          this->OnAuthorizationEvent.Broadcast(
              ECesiumITwinDelegateType::Failure,
              FString(),
              nullptr,
              Errors);
        } else {
          TSharedPtr<CesiumITwinClient::Connection> pInternalConnection =
              MakeShared<CesiumITwinClient::Connection>(
                  MoveTemp(*connection.value));
          UCesiumITwinConnection* pConnection =
              NewObject<UCesiumITwinConnection>();
          pConnection->SetConnection(pInternalConnection);
          this->OnAuthorizationEvent.Broadcast(
              ECesiumITwinDelegateType::Success,
              FString(),
              pConnection,
              TArray<FString>());
          this->SetReadyToDestroy();
        }
      });
}

UCesiumITwinAPIGetProfileAsyncAction*
UCesiumITwinAPIGetProfileAsyncAction::GetProfile(
    UCesiumITwinConnection* pConnection) {
  UCesiumITwinAPIGetProfileAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIGetProfileAsyncAction>();
  pAsyncAction->pConnection = pConnection->pConnection;
  return pAsyncAction;
}

void UCesiumITwinAPIGetProfileAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnProfileResult.Broadcast(nullptr, Errors);
    return;
  }

  this->pConnection->me().thenInMainThread([this](CesiumUtility::Result<
                                                  CesiumITwinClient::
                                                      UserProfile>&& result) {
    if (!IsValid(this)) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Authorization finished but authorize async action is no longer valid."));
      return;
    }

    if (!result.value) {
      TArray<FString> Errors;
      for (const std::string& error : result.errors.errors) {
        Errors.Emplace(UTF8_TO_TCHAR(error.c_str()));
      }

      for (const std::string& warning : result.errors.warnings) {
        Errors.Emplace(UTF8_TO_TCHAR(warning.c_str()));
      }

      OnProfileResult.Broadcast(nullptr, Errors);
    } else {
      UCesiumITwinUserProfile* pProfile = NewObject<UCesiumITwinUserProfile>();
      pProfile->SetProfile(std::move(*result.value));
      OnProfileResult.Broadcast(pProfile, TArray<FString>());
    }
  });
}

UCesiumITwinAPIGetResourcesAsyncAction*
UCesiumITwinAPIGetResourcesAsyncAction::GetResources(
    const UObject* WorldContextObject,
    UCesiumITwinConnection* pConnection) {
  UCesiumITwinAPIGetResourcesAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIGetResourcesAsyncAction>(
          WorldContextObject->GetWorld());
  pAsyncAction->pConnection = pConnection->pConnection;
  return pAsyncAction;
}

void UCesiumITwinAPIGetResourcesAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnResourcesEvent.Broadcast(
        EGetResourcesCallbackType::Failure,
        TArray<UCesiumITwinResource*>(),
        0,
        0,
        Errors);
    return;
  }

  this->pConnection
      ->listAllAvailableResources([this](
                                      const std::atomic_int& finished,
                                      const std::atomic_int& total) {
        AsyncTask(ENamedThreads::GameThread, [&finished, &total, this]() {
          this->OnResourcesEvent.Broadcast(
              EGetResourcesCallbackType::Status,
              {},
              finished.load(),
              total.load(),
              {});
        });
      })
      .thenInMainThread([this](CesiumUtility::Result<
                               std::vector<CesiumITwinClient::ITwinResource>>&&
                                   result) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Authorization finished but authorize async action is no longer valid."));
          return;
        }

        if (!result.value) {
          TArray<FString> Errors;
          for (const std::string& error : result.errors.errors) {
            Errors.Emplace(UTF8_TO_TCHAR(error.c_str()));
          }

          for (const std::string& warning : result.errors.warnings) {
            Errors.Emplace(UTF8_TO_TCHAR(warning.c_str()));
          }

          this->OnResourcesEvent
              .Broadcast(EGetResourcesCallbackType::Failure, {}, 0, 0, Errors);
        } else {
          TArray<UCesiumITwinResource*> Resources;
          Resources.Reserve(result.value->size());

          for (const CesiumITwinClient::ITwinResource& resource :
               *result.value) {
            UCesiumITwinResource* pNewResource =
                NewObject<UCesiumITwinResource>(GetWorld());
            pNewResource->SetResource(resource);
            pNewResource->SetConnection(this->pConnection);
            Resources.Push(pNewResource);
          }

          this->OnResourcesEvent.Broadcast(
              EGetResourcesCallbackType::Success,
              Resources,
              0,
              0,
              {});
        }
      });
}
