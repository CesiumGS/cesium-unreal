// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "UnrealAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "HAL/PlatformFileManager.h"
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

namespace {

struct AsyncReadRequestResult {
  std::unique_ptr<IAsyncReadFileHandle> readFileHandle;
  std::unique_ptr<IAsyncReadRequest> readRequest;
};

struct FileCallbackHolder {
  IAsyncReadFileHandle* readFileHandle;
  CesiumAsync::Promise<AsyncReadRequestResult> promise;
  FAsyncFileCallBack* callback;

  void operator()(bool bWasCancelled, IAsyncReadRequest* ReadRequest) {
    // Guarantee the callback will be destroyed after this function ends.
    std::unique_ptr<FAsyncFileCallBack> me(this->callback);
    this->callback = nullptr;

    // Guarantee the IAsyncReadRequest and readFileHandle are destroyed, too,
    // unless we pass them along.
    std::unique_ptr<IAsyncReadRequest> ReadRequestUnique(ReadRequest);
    std::unique_ptr<IAsyncReadFileHandle> ReadFileHandle(this->readFileHandle);
    this->readFileHandle = nullptr;

    if (bWasCancelled) {
      promise.reject(std::runtime_error("File request was canceled."));
    } else if (!ReadRequestUnique) {
      promise.reject(std::runtime_error("File request failed."));
    } else {
      promise.resolve(
          {std::move(ReadFileHandle), std::move(ReadRequestUnique)});
    }
  }

  static FAsyncFileCallBack* Create(
      std::unique_ptr<IAsyncReadFileHandle>&& ReadFileHandle,
      const CesiumAsync::Promise<AsyncReadRequestResult>& Promise) {
    FAsyncFileCallBack* Result = new FAsyncFileCallBack();
    *Result = FAsyncFileCallBack(
        FileCallbackHolder{ReadFileHandle.release(), Promise, Result});
    return Result;
  }
};

class UnrealFileAssetRequestResponse : public CesiumAsync::IAssetRequest,
                                       public CesiumAsync::IAssetResponse {
public:
  UnrealFileAssetRequestResponse(std::string&& url, uint8* data, int64 dataSize)
      : _url(std::move(url)), _data(data), _dataSize(dataSize) {}
  ~UnrealFileAssetRequestResponse() { delete[] this->_data; }

  virtual const std::string& method() const { return getMethod; }

  virtual const std::string& url() const { return this->_url; }

  virtual const CesiumAsync::HttpHeaders& headers() const override {
    return emptyHeaders;
  }

  virtual const CesiumAsync::IAssetResponse* response() const override {
    return this;
  }

  virtual uint16_t statusCode() const override {
    return this->_data == nullptr ? 404 : 200;
  }

  virtual std::string contentType() const override { return std::string(); }

  virtual gsl::span<const std::byte> data() const override {
    return gsl::span<const std::byte>(
        reinterpret_cast<const std::byte*>(this->_data),
        this->_dataSize);
  }

private:
  static const std::string getMethod;
  static const CesiumAsync::HttpHeaders emptyHeaders;

  std::string _url;
  uint8* _data;
  int64 _dataSize;
};

const std::string UnrealFileAssetRequestResponse::getMethod = "GET";
const CesiumAsync::HttpHeaders UnrealFileAssetRequestResponse::emptyHeaders{};

} // namespace

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnrealAssetAccessor::getFromFile(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) {
  check(!url.empty());

  return asyncSystem
      .createFuture<AsyncReadRequestResult>([&](const auto& promise) {
        FString filename =
            UTF8_TO_TCHAR(url.substr(sizeof(fileProtocol) - 1).c_str());

        // If this filename is now a relative path, make it absolute by
        // adding a slash at the start. For example,
        // `file:///home/foo/tileset.json` becomes `home/foo/tileset.json`
        // which is still relative, so we turn it into
        // `/home/foo/tileset.json`. FPaths::IsRelative correctly
        // identifies paths like `C:/foo/tileset.json` as absolute.
        if (FPaths::IsRelative(filename)) {
          filename = "/" + filename;
        }

        IPlatformFile& FileManager =
            FPlatformFileManager::Get().GetPlatformFile();
        std::unique_ptr<IAsyncReadFileHandle> ReadHandle(
            FileManager.OpenAsyncRead(*filename));
        if (!ReadHandle) {
          promise.reject(std::runtime_error("OpenAsyncRead failed."));
          return;
        }

        IAsyncReadFileHandle* RawReadHandle = ReadHandle.get();
        RawReadHandle->SizeRequest(
            FileCallbackHolder::Create(std::move(ReadHandle), promise));
      })
      .thenImmediately(
          [asyncSystem, url = url](AsyncReadRequestResult&& Result) mutable
          -> CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> {
            int64 size = Result.readRequest->GetSizeResults();
            if (size < 0) {
              // Indicates the file was not found or could not be read.
              return asyncSystem.createResolvedFuture<
                  std::shared_ptr<CesiumAsync::IAssetRequest>>(
                  std::make_shared<UnrealFileAssetRequestResponse>(
                      std::move(url),
                      nullptr,
                      0));
            }

            CesiumAsync::Promise<AsyncReadRequestResult> promise =
                asyncSystem.createPromise<AsyncReadRequestResult>();
            IAsyncReadFileHandle* readFileHandle = Result.readFileHandle.get();
            readFileHandle->ReadRequest(
                0,
                size,
                AIOP_Normal,
                FileCallbackHolder::Create(
                    std::move(Result.readFileHandle),
                    promise));
            return promise.getFuture().thenImmediately(
                [url = std::move(url),
                 size](AsyncReadRequestResult&& Result) mutable
                -> std::shared_ptr<CesiumAsync::IAssetRequest> {
                  if (size < 0) {
                    // Indicates the file was not found or could not be read.
                    return std::make_shared<UnrealFileAssetRequestResponse>(
                        std::move(url),
                        nullptr,
                        0);
                  } else {
                    uint8_t* data = Result.readRequest->GetReadResults();
                    return std::make_shared<UnrealFileAssetRequestResponse>(
                        std::move(url),
                        data,
                        size);
                  }
                });
          });
}
