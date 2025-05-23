#include "CesiumITwinAPIBlueprintLibrary.h"
#include "Async/Async.h"
#include "CesiumITwinCesiumCuratedContentRasterOverlay.h"
#include "CesiumRuntime.h"
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Result.h>

namespace {
TArray<FString> errorListToArray(const CesiumUtility::ErrorList& errors) {
  TArray<FString> Errors;
  for (const std::string& error : errors.errors) {
    Errors.Emplace(UTF8_TO_TCHAR(error.c_str()));
  }

  for (const std::string& warning : errors.warnings) {
    Errors.Emplace(UTF8_TO_TCHAR(warning.c_str()));
  }

  return Errors;
}
} // namespace

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
            ECesiumITwinAuthorizationDelegateType::OpenUrl,
            UTF8_TO_TCHAR(url.c_str()),
            nullptr,
            TArray<FString>());
      })
      .thenInMainThread([this](CesiumUtility::Result<
                               CesiumUtility::IntrusivePointer<
                                   CesiumITwinClient::Connection>>&&
                                   connection) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Authorization finished but authorize async action is no longer valid."));
          return;
        }

        if (!connection.pValue) {
          TArray<FString> Errors;
          for (const std::string& error : connection.errors.errors) {
            Errors.Emplace(UTF8_TO_TCHAR(error.c_str()));
          }

          for (const std::string& warning : connection.errors.warnings) {
            Errors.Emplace(UTF8_TO_TCHAR(warning.c_str()));
          }

          this->OnAuthorizationEvent.Broadcast(
              ECesiumITwinAuthorizationDelegateType::Failure,
              FString(),
              nullptr,
              errorListToArray(connection.errors));
        } else {
          UCesiumITwinConnection* pConnection =
              NewObject<UCesiumITwinConnection>();
          pConnection->SetConnection(connection.pValue);
          this->OnAuthorizationEvent.Broadcast(
              ECesiumITwinAuthorizationDelegateType::Success,
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
              "Get profile finished but get profile async action is no longer valid."));
      return;
    }

    if (!result.value) {
      OnProfileResult.Broadcast(nullptr, errorListToArray(result.errors));
    } else {
      UCesiumITwinUserProfile* pProfile = NewObject<UCesiumITwinUserProfile>();
      pProfile->SetProfile(std::move(*result.value));
      OnProfileResult.Broadcast(pProfile, TArray<FString>());
    }
  });
}

const int PageSize = 50;

UCesiumITwinAPIGetITwinsAsyncAction*
UCesiumITwinAPIGetITwinsAsyncAction::GetITwins(
    UCesiumITwinConnection* pConnection,
    int Page) {
  UCesiumITwinAPIGetITwinsAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIGetITwinsAsyncAction>();
  pAsyncAction->pConnection = pConnection->pConnection;
  pAsyncAction->page = FMath::Max(Page, 1);
  return pAsyncAction;
}

void UCesiumITwinAPIGetITwinsAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnITwinsResult.Broadcast(TArray<UCesiumITwin*>(), false, Errors);
    return;
  }

  CesiumITwinClient::QueryParameters params;
  params.top = PageSize;
  params.skip = PageSize * (this->page - 1);

  this->pConnection->itwins(params).thenInMainThread(
      [this](CesiumUtility::Result<
             CesiumITwinClient::PagedList<CesiumITwinClient::ITwin>>&& result) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Get itwins finished but get itwins async action is no longer valid."));
          return;
        }

        if (!result.value) {
          OnITwinsResult.Broadcast(
              TArray<UCesiumITwin*>(),
              false,
              errorListToArray(result.errors));
        } else {
          TArray<UCesiumITwin*> iTwins;
          iTwins.Reserve(result.value->size());
          for (CesiumITwinClient::ITwin& iTwin : *result.value) {
            UCesiumITwin* pITwin = NewObject<UCesiumITwin>();
            pITwin->SetITwin(MoveTemp(iTwin));
            iTwins.Emplace(pITwin);
          }
          OnITwinsResult.Broadcast(
              iTwins,
              result.value->hasNextUrl(),
              TArray<FString>());
        }
      });
}

UCesiumITwinAPIGetIModelsAsyncAction*
UCesiumITwinAPIGetIModelsAsyncAction::GetIModels(
    UCesiumITwinConnection* pConnection,
    const FString& iTwinId,
    int Page) {
  UCesiumITwinAPIGetIModelsAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIGetIModelsAsyncAction>();
  pAsyncAction->pConnection = pConnection->pConnection;
  pAsyncAction->page = FMath::Max(Page, 1);
  pAsyncAction->iTwinId = iTwinId;
  return pAsyncAction;
}

void UCesiumITwinAPIGetIModelsAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnIModelsResult.Broadcast(TArray<UCesiumIModel*>(), false, Errors);
    return;
  }

  CesiumITwinClient::QueryParameters params;
  params.top = PageSize;
  params.skip = PageSize * (this->page - 1);

  this->pConnection->imodels(TCHAR_TO_UTF8(*this->iTwinId), params)
      .thenInMainThread([this](
                            CesiumUtility::Result<CesiumITwinClient::PagedList<
                                CesiumITwinClient::IModel>>&& result) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Get imodels finished but get imodels async action is no longer valid."));
          return;
        }

        if (!result.value) {
          OnIModelsResult.Broadcast(
              TArray<UCesiumIModel*>(),
              false,
              errorListToArray(result.errors));
        } else {
          TArray<UCesiumIModel*> iModels;
          iModels.Reserve(result.value->size());
          for (CesiumITwinClient::IModel& iModel : *result.value) {
            UCesiumIModel* pIModel = NewObject<UCesiumIModel>();
            pIModel->SetIModel(MoveTemp(iModel));
            iModels.Emplace(pIModel);
          }
          OnIModelsResult.Broadcast(
              iModels,
              result.value->hasNextUrl(),
              TArray<FString>());
        }
      });
}

UCesiumITwinAPIGetIModelMeshExportsAsyncAction*
UCesiumITwinAPIGetIModelMeshExportsAsyncAction::GetIModelMeshExports(
    UCesiumITwinConnection* pConnection,
    const FString& iModelId,
    int Page) {
  UCesiumITwinAPIGetIModelMeshExportsAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIGetIModelMeshExportsAsyncAction>();
  pAsyncAction->pConnection = pConnection->pConnection;
  pAsyncAction->page = FMath::Max(Page, 1);
  pAsyncAction->iModelId = iModelId;
  return pAsyncAction;
}

void UCesiumITwinAPIGetIModelMeshExportsAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnIModelMeshExportsResult.Broadcast(
        TArray<UCesiumIModelMeshExport*>(),
        false,
        Errors);
    return;
  }

  CesiumITwinClient::QueryParameters params;
  params.top = PageSize;
  params.skip = PageSize * (this->page - 1);

  this->pConnection->meshExports(TCHAR_TO_UTF8(*this->iModelId), params)
      .thenInMainThread([this](
                            CesiumUtility::Result<CesiumITwinClient::PagedList<
                                CesiumITwinClient::IModelMeshExport>>&&
                                result) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Get imodel mesh exports finished but get imodel mesh exports async action is no longer valid."));
          return;
        }

        if (!result.value) {
          OnIModelMeshExportsResult.Broadcast(
              TArray<UCesiumIModelMeshExport*>(),
              false,
              errorListToArray(result.errors));
        } else {
          TArray<UCesiumIModelMeshExport*> iModels;
          iModels.Reserve(result.value->size());
          for (CesiumITwinClient::IModelMeshExport& iModel : *result.value) {
            UCesiumIModelMeshExport* pIModel =
                NewObject<UCesiumIModelMeshExport>();
            pIModel->SetIModelMeshExport(MoveTemp(iModel));
            iModels.Emplace(pIModel);
          }
          OnIModelMeshExportsResult.Broadcast(
              iModels,
              result.value->hasNextUrl(),
              TArray<FString>());
        }
      });
}

UCesiumITwinAPIGetRealityDataAsyncAction*
UCesiumITwinAPIGetRealityDataAsyncAction::GetITwinRealityData(
    UCesiumITwinConnection* pConnection,
    const FString& iTwinId,
    int Page) {
  UCesiumITwinAPIGetRealityDataAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIGetRealityDataAsyncAction>();
  pAsyncAction->pConnection = pConnection->pConnection;
  pAsyncAction->page = FMath::Max(Page, 1);
  pAsyncAction->iTwinId = iTwinId;
  return pAsyncAction;
}

void UCesiumITwinAPIGetRealityDataAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnITwinRealityDataResult.Broadcast(
        TArray<UCesiumITwinRealityData*>(),
        false,
        Errors);
    return;
  }

  CesiumITwinClient::QueryParameters params;
  params.top = PageSize;
  params.skip = PageSize * (this->page - 1);

  this->pConnection->realityData(TCHAR_TO_UTF8(*this->iTwinId), params)
      .thenInMainThread([this](
                            CesiumUtility::Result<CesiumITwinClient::PagedList<
                                CesiumITwinClient::ITwinRealityData>>&&
                                result) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Get reality data finished but get reality data async action is no longer valid."));
          return;
        }

        if (!result.value) {
          OnITwinRealityDataResult.Broadcast(
              TArray<UCesiumITwinRealityData*>(),
              false,
              errorListToArray(result.errors));
        } else {
          TArray<UCesiumITwinRealityData*> realityDataObjects;
          realityDataObjects.Reserve(result.value->size());
          for (CesiumITwinClient::ITwinRealityData& realityData :
               *result.value) {
            UCesiumITwinRealityData* pRealityData =
                NewObject<UCesiumITwinRealityData>();
            pRealityData->SetITwinRealityData(MoveTemp(realityData));
            realityDataObjects.Emplace(pRealityData);
          }
          OnITwinRealityDataResult.Broadcast(
              realityDataObjects,
              result.value->hasNextUrl(),
              TArray<FString>());
        }
      });
}

UCesiumITwinAPIListCesiumCuratedContentAsyncAction*
UCesiumITwinAPIListCesiumCuratedContentAsyncAction::
    GetCesiumCuratedContentAssets(UCesiumITwinConnection* pConnection) {
  UCesiumITwinAPIListCesiumCuratedContentAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIListCesiumCuratedContentAsyncAction>();
  pAsyncAction->pConnection = pConnection->pConnection;
  return pAsyncAction;
}

void UCesiumITwinAPIListCesiumCuratedContentAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnListCesiumCuratedContentDelegate.Broadcast(
        TArray<UCesiumCuratedContentAsset*>(),
        Errors);
    return;
  }

  this->pConnection->cesiumCuratedContent().thenInMainThread(
      [this](CesiumUtility::Result<std::vector<
                 CesiumITwinClient::CesiumCuratedContentAsset>>&& result) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Get cesium curated content finished but get cesium curated content async action is no longer valid."));
          return;
        }

        if (!result.value) {
          OnListCesiumCuratedContentDelegate.Broadcast(
              TArray<UCesiumCuratedContentAsset*>(),
              errorListToArray(result.errors));
        } else {
          TArray<UCesiumCuratedContentAsset*> assets;
          assets.Reserve(result.value->size());
          for (CesiumITwinClient::CesiumCuratedContentAsset& asset :
               *result.value) {
            UCesiumCuratedContentAsset* pAsset =
                NewObject<UCesiumCuratedContentAsset>();
            pAsset->SetCesiumCuratedContentAsset(MoveTemp(asset));
            assets.Emplace(pAsset);
          }
          OnListCesiumCuratedContentDelegate.Broadcast(
              assets,
              TArray<FString>());
        }
      });
}
