// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "UnrealAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include <cstddef>
#include <optional>
#include <set>

static CesiumAsync::HttpHeaders
parseHeaders(const TArray<FString>& unrealHeaders) {
  CesiumAsync::HttpHeaders result;
  for (const FString& header : unrealHeaders) {
    int32_t separator = -1;
    if (header.FindChar(':', separator)) {
      FString fstrKey = header.Left(separator);
      FString fstrValue = header.Right(header.Len() - separator - 2);
      std::string key = std::string(TCHAR_TO_UTF8(*fstrKey));
      std::string value = std::string(TCHAR_TO_UTF8(*fstrValue));
      result.insert({std::move(key), std::move(value)});
    }
  }

  return result;
}

class UnrealAssetResponse : public CesiumAsync::IAssetResponse {
public:
  UnrealAssetResponse(FHttpResponsePtr pResponse)
      : _pResponse(pResponse),
        _headers(parseHeaders(pResponse->GetAllHeaders())) {}

  virtual uint16_t statusCode() const override {
    return this->_pResponse->GetResponseCode();
  }

  virtual std::string contentType() const override {
    return TCHAR_TO_UTF8(*this->_pResponse->GetContentType());
  }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return this->_headers;
  }

  virtual gsl::span<const std::byte> data() const override {
    const TArray<uint8>& content = this->_pResponse->GetContent();
    return gsl::span(
        reinterpret_cast<const std::byte*>(content.GetData()),
        content.Num());
  }

private:
  FHttpResponsePtr _pResponse;
  CesiumAsync::HttpHeaders _headers;
};

class UnrealAssetRequest : public CesiumAsync::IAssetRequest {
public:
  UnrealAssetRequest(FHttpRequestPtr pRequest, FHttpResponsePtr pResponse)
      : _pRequest(pRequest),
        _pResponse(std::make_unique<UnrealAssetResponse>(pResponse)) {
    this->_headers = parseHeaders(this->_pRequest->GetAllHeaders());
    this->_url = TCHAR_TO_UTF8(*this->_pRequest->GetURL());
    this->_method = TCHAR_TO_UTF8(*this->_pRequest->GetVerb());
  }

  virtual const std::string& method() const { return this->_method; }

  virtual const std::string& url() const { return this->_url; }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return _headers;
  }

  virtual const CesiumAsync::IAssetResponse* response() const override {
    return this->_pResponse.get();
  }

private:
  FHttpRequestPtr _pRequest;
  std::unique_ptr<UnrealAssetResponse> _pResponse;
  std::string _url;
  std::string _method;
  CesiumAsync::HttpHeaders _headers;
};

UnrealAssetAccessor::UnrealAssetAccessor() : _userAgent() {
  FString OsVersion, OsSubVersion;
  FPlatformMisc::GetOSVersions(OsVersion, OsSubVersion);

  IPluginManager& PluginManager = IPluginManager::Get();
  TSharedPtr<IPlugin> pCesiumPlugin =
      PluginManager.FindPlugin("CesiumForUnreal");

  FString version = "unknown";
  if (pCesiumPlugin) {
    version = pCesiumPlugin->GetDescriptor().VersionName;
  }

  this->_userAgent = TEXT("Mozilla/5.0 (");
  this->_userAgent += OsVersion;
  this->_userAgent += " ";
  this->_userAgent += FPlatformMisc::GetOSVersion();
  this->_userAgent += TEXT(") Cesium For Unreal/");
  this->_userAgent += version;
  this->_userAgent += TEXT(" (Project ");
  this->_userAgent += FApp::GetProjectName();
  this->_userAgent += " Engine ";
  this->_userAgent += FEngineVersion::Current().ToString();
  this->_userAgent += TEXT(")");
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnrealAssetAccessor::requestAsset(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {

#if TRACING_ENABLED
  int64_t tracingID = CESIUM_TRACE_CURRENT_ASYNC_ID();
  if (tracingID >= 0) {
    CESIUM_TRACE_BEGIN_ID("requestAsset", tracingID);
  }
#else
  int64_t tracingID = -1;
#endif

  const FString& userAgent = this->_userAgent;

  return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
      [tracingID, &url, &headers, &userAgent](const auto& promise) {
        FHttpModule& httpModule = FHttpModule::Get();
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest =
            httpModule.CreateRequest();
        pRequest->SetURL(UTF8_TO_TCHAR(url.c_str()));

        for (const CesiumAsync::IAssetAccessor::THeader& header : headers) {
          pRequest->SetHeader(
              UTF8_TO_TCHAR(header.first.c_str()),
              UTF8_TO_TCHAR(header.second.c_str()));
        }

        pRequest->AppendToHeader(TEXT("User-Agent"), userAgent);

        pRequest->OnProcessRequestComplete().BindLambda(
            [promise, tracingID](
                FHttpRequestPtr pRequest,
                FHttpResponsePtr pResponse,
                bool connectedSuccessfully) {
              if (tracingID >= 0) {
                CESIUM_TRACE_END_ID("requestAsset", tracingID);
              }
              if (connectedSuccessfully) {
                promise.resolve(
                    std::make_unique<UnrealAssetRequest>(pRequest, pResponse));
              } else {
                switch (pRequest->GetStatus()) {
                case EHttpRequestStatus::Failed_ConnectionError:
                  promise.reject(std::runtime_error("Connection failed."));
                default:
                  promise.reject(std::runtime_error("Request failed."));
                }
              }
            });

        pRequest->ProcessRequest();
      });
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnrealAssetAccessor::post(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
    const gsl::span<const std::byte>& contentPayload) {

  const FString& userAgent = this->_userAgent;

  return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
      [&url, &headers, &userAgent, &contentPayload](const auto& promise) {
        FHttpModule& httpModule = FHttpModule::Get();
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest =
            httpModule.CreateRequest();
        pRequest->SetVerb(TEXT("POST"));
        pRequest->SetURL(UTF8_TO_TCHAR(url.c_str()));

        for (const CesiumAsync::IAssetAccessor::THeader& header : headers) {
          pRequest->SetHeader(
              UTF8_TO_TCHAR(header.first.c_str()),
              UTF8_TO_TCHAR(header.second.c_str()));
        }

        pRequest->AppendToHeader(TEXT("User-Agent"), userAgent);

        pRequest->SetContent(TArray<uint8>(
            reinterpret_cast<const uint8*>(contentPayload.data()),
            contentPayload.size()));

        pRequest->OnProcessRequestComplete().BindLambda(
            [promise](
                FHttpRequestPtr pRequest,
                FHttpResponsePtr pResponse,
                bool connectedSuccessfully) {
              if (connectedSuccessfully) {
                promise.resolve(
                    std::make_unique<UnrealAssetRequest>(pRequest, pResponse));
              } else {
                switch (pRequest->GetStatus()) {
                case EHttpRequestStatus::Failed_ConnectionError:
                  promise.reject(std::runtime_error("Connection failed."));
                default:
                  promise.reject(std::runtime_error("Request failed."));
                }
              }
            });

        pRequest->ProcessRequest();
      });
}

void UnrealAssetAccessor::tick() noexcept {
  FHttpManager& manager = FHttpModule::Get().GetHttpManager();
  manager.Tick(0.0f);
}
