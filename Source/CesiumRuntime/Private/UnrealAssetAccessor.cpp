// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "UnrealAssetAccessor.h"
#include "Async/Async.h"
#include "Async/AsyncWork.h"

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumCommon.h"
#include "CesiumRuntime.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include <cstddef>
#include <cstring>
#include <optional>
#include <set>
#include <uriparser/Uri.h>

namespace {

CesiumAsync::HttpHeaders parseHeaders(const TArray<FString>& unrealHeaders) {
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
    return static_cast<uint16_t>(this->_pResponse->GetResponseCode());
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

} // namespace

UnrealAssetAccessor::UnrealAssetAccessor()
    : _userAgent(), _cesiumRequestHeaders() {
  FString OsVersion, OsSubVersion;
  FPlatformMisc::GetOSVersions(OsVersion, OsSubVersion);
  OsVersion += " " + FPlatformMisc::GetOSVersion();

  IPluginManager& PluginManager = IPluginManager::Get();
  TSharedPtr<IPlugin> pCesiumPlugin =
      PluginManager.FindPlugin("CesiumForUnreal");

  FString version = "unknown";
  if (pCesiumPlugin) {
    version = pCesiumPlugin->GetDescriptor().VersionName;
  }

  const TCHAR* projectName = FApp::GetProjectName();
  FString engine = "Unreal Engine " + FEngineVersion::Current().ToString();

  this->_userAgent = TEXT("Mozilla/5.0 (");
  this->_userAgent += OsVersion;
  this->_userAgent += TEXT(") Cesium For Unreal/");
  this->_userAgent += version;
  this->_userAgent += TEXT(" (Project ");
  this->_userAgent += projectName;
  this->_userAgent += " Engine ";
  this->_userAgent += engine;
  this->_userAgent += TEXT(")");

  this->_cesiumRequestHeaders.Add(
      TEXT("X-Cesium-Client"),
      TEXT("Cesium For Unreal"));
  this->_cesiumRequestHeaders.Add(TEXT("X-Cesium-Client-Version"), version);
  this->_cesiumRequestHeaders.Add(TEXT("X-Cesium-Client-Project"), projectName);
  this->_cesiumRequestHeaders.Add(TEXT("X-Cesium-Client-Engine"), engine);
  this->_cesiumRequestHeaders.Add(TEXT("X-Cesium-Client-OS"), OsVersion);
}

namespace {

const char fileProtocol[] = "file:///";

bool isFile(const std::string& url) {
  return url.compare(0, sizeof(fileProtocol) - 1, fileProtocol) == 0;
}

void rejectPromiseOnUnsuccessfulConnection(
    const CesiumAsync::Promise<std::shared_ptr<CesiumAsync::IAssetRequest>>&
        promise,
    FHttpRequestPtr pRequest) {
#ifndef ENGINE_VERSION_5_4_OR_HIGHER
#define ENGINE_VERSION_5_4_OR_HIGHER 0
#endif

#if ENGINE_VERSION_5_4_OR_HIGHER
  if (pRequest->GetStatus() == EHttpRequestStatus::Failed) {
    EHttpFailureReason failureReason = pRequest->GetFailureReason();
    promise.reject(std::runtime_error(fmt::format(
        "Request failed: {}",
        TCHAR_TO_UTF8(LexToString(failureReason)))));
  } else {
    promise.reject(std::runtime_error(fmt::format(
        "Request not successful: {}",
        TCHAR_TO_UTF8(ToString(pRequest->GetStatus())))));
  }
#else
  if (pRequest->GetStatus() == EHttpRequestStatus::Failed_ConnectionError) {
    promise.reject(std::runtime_error("Connection failed."));
  } else {
    promise.reject(std::runtime_error("Request failed."));
  }
#endif
}

} // namespace

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnrealAssetAccessor::get(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {

  CESIUM_TRACE_BEGIN_IN_TRACK("requestAsset");

  if (isFile(url)) {
    return getFromFile(asyncSystem, url, headers);
  }

  const FString& userAgent = this->_userAgent;
  const TMap<FString, FString>& cesiumRequestHeaders =
      this->_cesiumRequestHeaders;

  return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
      [&url, &headers, &userAgent, &cesiumRequestHeaders](const auto& promise) {
        FHttpModule& httpModule = FHttpModule::Get();
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest =
            httpModule.CreateRequest();
        pRequest->SetURL(UTF8_TO_TCHAR(url.c_str()));

        for (const auto& header : headers) {
          pRequest->SetHeader(
              UTF8_TO_TCHAR(header.first.c_str()),
              UTF8_TO_TCHAR(header.second.c_str()));
        }

        for (const auto& header : cesiumRequestHeaders) {
          pRequest->SetHeader(header.Key, header.Value);
        }

        pRequest->AppendToHeader(TEXT("User-Agent"), userAgent);

        pRequest->OnProcessRequestComplete().BindLambda(
            [promise, CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()](
                FHttpRequestPtr pRequest,
                FHttpResponsePtr pResponse,
                bool connectedSuccessfully) mutable {
              CESIUM_TRACE_USE_CAPTURED_TRACK();
              CESIUM_TRACE_END_IN_TRACK("requestAsset");

              if (connectedSuccessfully) {
                promise.resolve(
                    std::make_unique<UnrealAssetRequest>(pRequest, pResponse));
              } else {
                rejectPromiseOnUnsuccessfulConnection(promise, pRequest);
              }
            });

        pRequest->ProcessRequest();
      });
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnrealAssetAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
    const gsl::span<const std::byte>& contentPayload) {

  const FString& userAgent = this->_userAgent;
  const TMap<FString, FString>& cesiumRequestHeaders =
      this->_cesiumRequestHeaders;

  return asyncSystem.createFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(
      [&verb,
       &url,
       &headers,
       &userAgent,
       &cesiumRequestHeaders,
       &contentPayload](const auto& promise) {
        FHttpModule& httpModule = FHttpModule::Get();
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest =
            httpModule.CreateRequest();
        pRequest->SetVerb(UTF8_TO_TCHAR(verb.c_str()));
        pRequest->SetURL(UTF8_TO_TCHAR(url.c_str()));

        for (const auto& header : headers) {
          pRequest->SetHeader(
              UTF8_TO_TCHAR(header.first.c_str()),
              UTF8_TO_TCHAR(header.second.c_str()));
        }

        for (const auto& header : cesiumRequestHeaders) {
          pRequest->SetHeader(header.Key, header.Value);
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
                rejectPromiseOnUnsuccessfulConnection(promise, pRequest);
              }
            });

        pRequest->ProcessRequest();
      });
}

void UnrealAssetAccessor::tick() noexcept {
  FHttpManager& manager = FHttpModule::Get().GetHttpManager();
  manager.Tick(0.0f);
}

namespace {

class UnrealFileAssetRequestResponse : public CesiumAsync::IAssetRequest,
                                       public CesiumAsync::IAssetResponse {
public:
  UnrealFileAssetRequestResponse(
      std::string&& url,
      uint16_t statusCode,
      TArray64<uint8>&& data)
      : _url(std::move(url)), _statusCode(statusCode), _data(data) {}

  virtual const std::string& method() const { return getMethod; }

  virtual const std::string& url() const { return this->_url; }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return emptyHeaders;
  }

  virtual const CesiumAsync::IAssetResponse* response() const override {
    return this;
  }

  virtual uint16_t statusCode() const override { return this->_statusCode; }

  virtual std::string contentType() const override { return std::string(); }

  virtual gsl::span<const std::byte> data() const override {
    return gsl::span<const std::byte>(
        reinterpret_cast<const std::byte*>(this->_data.GetData()),
        size_t(this->_data.Num()));
  }

private:
  static const std::string getMethod;
  static const CesiumAsync::HttpHeaders emptyHeaders;

  std::string _url;
  uint16_t _statusCode;
  TArray64<uint8> _data;
};

const std::string UnrealFileAssetRequestResponse::getMethod = "GET";
const CesiumAsync::HttpHeaders UnrealFileAssetRequestResponse::emptyHeaders{};

std::string convertFileUriToFilename(const std::string& url) {
  // According to the uriparser docs, both uriUriStringToWindowsFilenameA and
  // uriUriStringToUnixFilenameA require an output buffer with space for at most
  // length(url)+1 characters.
  // https://uriparser.github.io/doc/api/latest/Uri_8h.html#a4afbc8453c7013b9618259bc57d81a39
  std::string result(url.size() + 1, '\0');

#ifdef _WIN32
  int errorCode = uriUriStringToWindowsFilenameA(url.c_str(), result.data());
#else
  int errorCode = uriUriStringToUnixFilenameA(url.c_str(), result.data());
#endif

  // Truncate the string if necessary by finding the first null character.
  size_t end = result.find('\0');
  if (end != std::string::npos) {
    result.resize(end);
  }

  // Remove query parameters from the URL if present, as they are no longer
  // ignored by Unreal.
  size_t pos = result.find("?");
  if (pos != std::string::npos) {
    result.erase(pos);
  }

  return result;
}

class FCesiumReadFileWorker : public FNonAbandonableTask {
public:
  FCesiumReadFileWorker(
      const std::string& url,
      const CesiumAsync::AsyncSystem& asyncSystem)
      : _url(url),
        _promise(
            asyncSystem
                .createPromise<std::shared_ptr<CesiumAsync::IAssetRequest>>()) {
  }

  FORCEINLINE TStatId GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(
        FCesiumReadFileWorker,
        STATGROUP_ThreadPoolAsyncTasks);
  }

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> GetFuture() {
    return this->_promise.getFuture();
  }

  void DoWork() {
    FString filename =
        UTF8_TO_TCHAR(convertFileUriToFilename(this->_url).c_str());
    TArray64<uint8> data;
    if (FFileHelper::LoadFileToArray(data, *filename)) {
      this->_promise.resolve(std::make_shared<UnrealFileAssetRequestResponse>(
          std::move(this->_url),
          200,
          std::move(data)));
    } else {
      this->_promise.resolve(std::make_shared<UnrealFileAssetRequestResponse>(
          std::move(this->_url),
          404,
          TArray64<uint8>()));
    }
  }

private:
  std::string _url;
  CesiumAsync::Promise<std::shared_ptr<CesiumAsync::IAssetRequest>> _promise;
};

} // namespace

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnrealAssetAccessor::getFromFile(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
  check(!url.empty());

  auto pTaskOwner =
      std::make_unique<FAsyncTask<FCesiumReadFileWorker>>(url, asyncSystem);

  FAsyncTask<FCesiumReadFileWorker>* pTask = pTaskOwner.get();

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> future =
      pTask->GetTask().GetFuture().thenInWorkerThread(
          [pTaskOwner = std::move(pTaskOwner)](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
            // This lambda, via its capture, keeps the task instance alive until
            // it is complete.
            pTaskOwner->EnsureCompletion(false, false);
            return pRequest;
          });

  pTask->StartBackgroundTask(GIOThreadPool);

  return future;
}
