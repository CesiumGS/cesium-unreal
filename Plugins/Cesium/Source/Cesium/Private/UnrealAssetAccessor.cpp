#include "UnrealAssetAccessor.h"
#include "Cesium3DTiles/IAssetRequest.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UnrealConversions.h"
#include <optional>

class UnrealAssetResponse : public Cesium3DTiles::IAssetResponse {
public:
	UnrealAssetResponse(FHttpResponsePtr pResponse) :
		_pResponse(pResponse)
	{
	}

	virtual uint16_t statusCode() override {
		return this->_pResponse->GetResponseCode();
	}

	//virtual const std::map<std::string, std::string>& headers() override {
	//	// TODO: return the headers
	//	return this->_headers;
	//}

	virtual gsl::span<const uint8_t> data() override	{
		const TArray<uint8>& content = this->_pResponse->GetContent();
		return gsl::span(static_cast<const uint8_t*>(&content[0]), content.Num());
	}

private:
	FHttpResponsePtr _pResponse;
	//std::map<std::string, std::string> _headers;
};

class UnrealAssetRequest : public Cesium3DTiles::IAssetRequest {
public:
	UnrealAssetRequest(const std::string& url) :
		_pRequest(nullptr),
		_pResponse(nullptr),
		_callback()
	{
		FHttpModule& httpModule = FHttpModule::Get();
		this->_pRequest = httpModule.CreateRequest();
		this->_pRequest->SetURL(utf8_to_wstr(url));
		this->_pRequest->OnProcessRequestComplete().BindRaw(this, &UnrealAssetRequest::responseReceived);
		this->_pRequest->ProcessRequest();
	}

	virtual ~UnrealAssetRequest() {
		this->_pRequest->OnProcessRequestComplete().Unbind();
		this->_pRequest->CancelRequest();
	}

	virtual std::string url() const {
		return wstr_to_utf8(this->_pRequest->GetURL());
	}

	virtual Cesium3DTiles::IAssetResponse* response() override {
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

	void bind(std::function<void (IAssetRequest*)> callback) {
		this->_callback = callback;
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
	Cesium3DTiles::IAssetResponse* _pResponse;
	std::function<void (Cesium3DTiles::IAssetRequest*)> _callback;
};

std::unique_ptr<Cesium3DTiles::IAssetRequest> UnrealAssetAccessor::requestAsset(const std::string& url) {
	return std::make_unique<UnrealAssetRequest>(url);
}
