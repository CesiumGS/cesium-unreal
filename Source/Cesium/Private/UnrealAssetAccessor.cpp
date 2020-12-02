#include "UnrealAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumAsync/IAssetResponse.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UnrealConversions.h"
#include <optional>

class UnrealAssetResponse : public CesiumAsync::IAssetResponse {
public:
	UnrealAssetResponse(FHttpResponsePtr pResponse) :
		_pResponse(pResponse)
	{
	}

	virtual uint16_t statusCode() const override {
		return this->_pResponse->GetResponseCode();
	}

	virtual std::string contentType() const override {
		return wstr_to_utf8(this->_pResponse->GetContentType());
	}

	//virtual const std::map<std::string, std::string>& headers() override {
	//	// TODO: return the headers
	//	return this->_headers;
	//}

	virtual gsl::span<const uint8_t> data() const override	{
		const TArray<uint8>& content = this->_pResponse->GetContent();
		return gsl::span(static_cast<const uint8_t*>(content.GetData()), content.Num());
	}

private:
	FHttpResponsePtr _pResponse;
	//std::map<std::string, std::string> _headers;
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

	virtual CesiumAsync::IAssetResponse* response() override {
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
	CesiumAsync::IAssetResponse* _pResponse;
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
