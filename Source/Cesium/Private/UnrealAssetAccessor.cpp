#include "UnrealAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "UnrealConversions.h"
#include <set>
#include <optional>

static CesiumAsync::HttpHeaders parseHeaders(const TArray<FString>& unrealHeaders) {
	CesiumAsync::HttpHeaders result;
	for (const FString& header : unrealHeaders) {
		int32_t separator = -1;
		if (header.FindChar(':', separator)) {
			FString fstrKey = header.Left(separator);
			FString fstrValue = header.Right(header.Len() - separator - 2);
			std::string key = std::string(TCHAR_TO_UTF8(*fstrKey));
			std::string value = std::string(TCHAR_TO_UTF8(*fstrValue));
			result.insert({ std::move(key), std::move(value) });
		}
	}

	return result;
}

class UnrealAssetResponse : public CesiumAsync::IAssetResponse {
public:
	UnrealAssetResponse(FHttpResponsePtr pResponse) :
		_pResponse(pResponse),
		_cacheControl{}
	{
		this->_headers = parseHeaders(this->_pResponse->GetAllHeaders());
		this->_contentType = wstr_to_utf8(this->_pResponse->GetContentType());
		CesiumAsync::HttpHeaders::const_iterator cacheControlIter = _headers.find("Cache-Control");
		if (cacheControlIter != _headers.end()) {
			this->_cacheControl = CesiumAsync::ResponseCacheControl::parseFromResponseHeaders(_headers);
		}
	}

	virtual uint16_t statusCode() const override {
		return this->_pResponse->GetResponseCode();
	}

	virtual const std::string& contentType() const override {
		return this->_contentType;
	}

	virtual const CesiumAsync::HttpHeaders& headers() const override {
		return _headers;
	}

	virtual const CesiumAsync::ResponseCacheControl *cacheControl() const override {
		return _cacheControl != std::nullopt ? &(*_cacheControl) : nullptr;
	}

	virtual gsl::span<const uint8_t> data() const override	{
		const TArray<uint8>& content = this->_pResponse->GetContent();
		return gsl::span(static_cast<const uint8_t*>(content.GetData()), content.Num());
	}

private:
	FHttpResponsePtr _pResponse;
	std::string _contentType;
	CesiumAsync::HttpHeaders _headers;
	std::optional<CesiumAsync::ResponseCacheControl> _cacheControl;
};

class UnrealAssetRequest : public CesiumAsync::IAssetRequest {
public:
	UnrealAssetRequest(FHttpRequestPtr pRequest, FHttpResponsePtr pResponse) 
		: _pRequest(pRequest)
		, _pResponse(std::make_unique<UnrealAssetResponse>(pResponse))
	{
		this->_headers = parseHeaders(this->_pRequest->GetAllHeaders());
		this->_url = wstr_to_utf8(this->_pRequest->GetURL());
		this->_method = wstr_to_utf8(this->_pRequest->GetVerb());
	}

	virtual const std::string& method() const {
		return this->_method;
	}

	virtual const std::string& url() const {
		return this->_url;
	}

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

void UnrealAssetAccessor::requestAsset(
	const CesiumAsync::AsyncSystem* /*asyncSystem*/,
	const std::string& url,
	const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
	std::function<void(std::shared_ptr<CesiumAsync::IAssetRequest>)> callback) 
{
	FHttpModule& httpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest = httpModule.CreateRequest();
	pRequest->SetURL(utf8_to_wstr(url));
	
	for (const CesiumAsync::IAssetAccessor::THeader& header : headers) {
		pRequest->SetHeader(utf8_to_wstr(header.first), utf8_to_wstr(header.second));
	}

	pRequest->AppendToHeader(TEXT("User-Agent"), TEXT("Cesium for Unreal Engine"));
	pRequest->OnProcessRequestComplete().BindLambda([callback](FHttpRequestPtr pRequest, FHttpResponsePtr pResponse, bool connectedSuccessfully) {
		if (connectedSuccessfully) {
			callback(std::make_unique<UnrealAssetRequest>(pRequest, pResponse));
		}
	});

	pRequest->ProcessRequest();
}

void UnrealAssetAccessor::tick() noexcept {
	FHttpManager& manager = FHttpModule::Get().GetHttpManager();
	manager.Tick(0.0f);
}

