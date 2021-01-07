#include "UnrealAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UnrealConversions.h"
#include <set>
#include <optional>

class UnrealAssetResponse : public CesiumAsync::IAssetResponse {
public:
	UnrealAssetResponse(FHttpResponsePtr pResponse) :
		_cacheControl{},
		_pResponse(pResponse)
	{
		TArray<FString> headers = this->_pResponse->GetAllHeaders();
		for (const FString& header : headers) {
			int32_t separator = -1;
			if (header.FindChar(':', separator)) {
				FString fstrKey = header.Left(separator);
				FString fstrValue = header.Right(header.Len() - separator - 2);
				std::string key = std::string(TCHAR_TO_UTF8(*fstrKey));
				std::string value = std::string(TCHAR_TO_UTF8(*fstrValue));
				parseHeaderForCacheControl(key, value);
				_headers.insert({ std::move(key), std::move(value) });
			}
		}
	}

	virtual uint16_t statusCode() const override {
		return this->_pResponse->GetResponseCode();
	}

	virtual std::string contentType() const override {
		return wstr_to_utf8(this->_pResponse->GetContentType());
	}

	virtual const std::map<std::string, std::string> &headers() const override {
		return _headers;
	}

	virtual const CacheControl &cacheControl() const override {
		return _cacheControl;
	}

	virtual gsl::span<const uint8_t> data() const override	{
		const TArray<uint8>& content = this->_pResponse->GetContent();
		return gsl::span(static_cast<const uint8_t*>(content.GetData()), content.Num());
	}

private:
	void parseHeaderForCacheControl(const std::string& headerKey, const std::string& headerValue) {
		if (headerKey != "Cache-Control") {
			return;
		}

		std::set<std::string> directives;
		size_t last = 0;
		size_t next = 0;
		while ((next = headerValue.find(",", last)) != std::string::npos) {
			directives.insert(headerValue.substr(last, next - last));
			last = next + 1;
		}
		directives.insert(headerValue.substr(last));

		_cacheControl.mustRevalidate = directives.find("must-revalidate") != directives.end();
		_cacheControl.noCache = directives.find("no-cache") != directives.end();
		_cacheControl.noStore = directives.find("no-store") != directives.end();
		_cacheControl.noTransform = directives.find("no-transform") != directives.end();
		_cacheControl.accessControlPublic = directives.find("public") != directives.end();
		_cacheControl.accessControlPrivate = directives.find("private") != directives.end();
		_cacheControl.proxyRevalidate = directives.find("proxyRevalidate") != directives.end();
	}

	CacheControl _cacheControl;
	FHttpResponsePtr _pResponse;
	std::map<std::string, std::string> _headers;
};

class UnrealAssetRequest : public CesiumAsync::IAssetRequest {
public:
	UnrealAssetRequest(const std::string& url, const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers) :
		_pRequest(nullptr),
		_pResponse(nullptr),
		_callback()
	{
		FHttpModule& httpModule = FHttpModule::Get();
		this->_pRequest = httpModule.CreateRequest();
		this->_pRequest->SetURL(utf8_to_wstr(url));
		
		for (const CesiumAsync::IAssetAccessor::THeader& header : headers) {
			this->_pRequest->SetHeader(utf8_to_wstr(header.first), utf8_to_wstr(header.second));
		}

		this->_pRequest->AppendToHeader(TEXT("User-Agent"), TEXT("Cesium for Unreal Engine"));

		this->_pRequest->OnProcessRequestComplete().BindRaw(this, &UnrealAssetRequest::responseReceived);
		this->_pRequest->ProcessRequest();
	}

	virtual ~UnrealAssetRequest() {
		this->_pRequest->OnProcessRequestComplete().Unbind();
		this->cancel();
	}

	virtual std::string url() const {
		return wstr_to_utf8(this->_pRequest->GetURL());
	}

	virtual const CesiumAsync::IAssetResponse* response() const override {
		if (this->_pResponse) {
			return this->_pResponse;
		}

		EHttpRequestStatus::Type status = this->_pRequest->GetStatus();
		if (status == EHttpRequestStatus::Type::Succeeded) {
			this->_pResponse = new UnrealAssetResponse(this->_pRequest->GetResponse());
		} else if (status == EHttpRequestStatus::Type::Failed || status == EHttpRequestStatus::Type::Failed_ConnectionError) {
			// TODO: report failures
		}

		return this->_pResponse;
	}

	virtual void bind(std::function<void (IAssetRequest*)> callback) {
		this->_callback = callback;
	}

	virtual void cancel() noexcept {
		// Only cancel the request if the HttpManager still has it.
		// If not, it means the request already completed or canceled, and Unreal
		// will (for some reason!) re-invoke OnProcessRequestComplete if we
		// cancel now.
		FHttpManager& manager = FHttpModule::Get().GetHttpManager();
		if (manager.IsValidRequest(this->_pRequest.Get())) {
			this->_pRequest->CancelRequest();
		}
	}

protected:
	void responseReceived(FHttpRequestPtr pRequest, FHttpResponsePtr pResponse, bool connectedSuccessfully) {
		// This will be raised in the UE game thread when the response is received.
		if (this->_callback) {
			this->_callback(this);
		}
	}

private:
	FHttpRequestPtr _pRequest;
	mutable CesiumAsync::IAssetResponse* _pResponse;
	std::function<void (CesiumAsync::IAssetRequest*)> _callback;
};

std::unique_ptr<CesiumAsync::IAssetRequest> UnrealAssetAccessor::requestAsset(
	const std::string& url,
	const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers
) {
	return std::make_unique<UnrealAssetRequest>(url, headers);
}

void UnrealAssetAccessor::tick() noexcept {
	FHttpManager& manager = FHttpModule::Get().GetHttpManager();
	manager.Tick(0.0f);
}
