#include "EncryptAssetAccessor.h"

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetResponse.h"
#include "EncryptionUtility.h"
#include "Misc/Paths.h"
#include "CesiumRuntimeSettings.h"

namespace  {

class DecryptAssetResponse:public CesiumAsync::IAssetResponse {
public:
  DecryptAssetResponse(const IAssetResponse* pOther)noexcept
  :_pAssetResponse{pOther} {
    Decrypt();
  }
  virtual uint16_t statusCode() const noexcept override {
    return this->_pAssetResponse->statusCode();
  }

  virtual std::string contentType() const override {
    return this->_pAssetResponse->contentType();
  }

  virtual const CesiumAsync::HttpHeaders& headers() const noexcept override {
    return this->_pAssetResponse->headers();
  }

  virtual gsl::span<const std::byte> data() const noexcept override {
    return this->_dataValid ? this->_decryptedData
                            : this->_pAssetResponse->data();
  }
private:
  const CesiumAsync::IAssetResponse* _pAssetResponse;
  std::string SData;
  gsl::span<const std::byte>_decryptedData;
  bool _dataValid=false;
  FString AESKey=GetMutableDefault<UCesiumRuntimeSettings>()->AesKey;
  FString IV= GetMutableDefault<UCesiumRuntimeSettings>()->IV;
  const FString AESKeyPath = GetMutableDefault<UCesiumRuntimeSettings>()->AESKeyPath;
  const FString RSAPrivKeyPath = GetMutableDefault<UCesiumRuntimeSettings>()->RSAPrivateKeyPath;
  void Decrypt() {
    auto encryptedHeader=this->headers().find("Encrypted");
    if (encryptedHeader != headers().end()) {
      if(GetMutableDefault<UCesiumRuntimeSettings>()->UseAesKeyFile) {
        AESKey=UEncryptionUtility::GetAESKeyByFile(AESKeyPath);
      }
      _dataValid = true;
      switch(auto s=encryptedHeader->second.at(0))
      {
      case '1':
        SData=UEncryptionUtility::S_RSADecryptData(_pAssetResponse->data(),RSAPrivKeyPath);
        break;
      case '2':
        SData=UEncryptionUtility::S_CBC_AESDecryptData(_pAssetResponse->data(),AESKey,IV);
        break;
      case '3':
        SData=UEncryptionUtility::S_ECB_AESDecryptData(_pAssetResponse->data(),AESKey);
        break;
      default:
        _dataValid=false;
      }
      if(_dataValid) {
        _decryptedData=gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(SData.data()),SData.size());
      }
    }else {
      _dataValid=false;
    }
  }
};

class DecryptAssetRequest:public CesiumAsync::IAssetRequest {
public:
  DecryptAssetRequest(std::shared_ptr<IAssetRequest>&& pOther)
  :_pAssetRequest(std::move(pOther))
  ,_AssetResponse(_pAssetRequest->response()){};
  virtual const std::string& method() const noexcept override {
    return this->_pAssetRequest->method();
  }

  virtual const std::string& url() const noexcept override {
    return this->_pAssetRequest->url();
  }

  virtual const CesiumAsync::HttpHeaders& headers() const noexcept override {
    return this->_pAssetRequest->headers();
  }

  virtual const CesiumAsync::IAssetResponse* response() const noexcept override {
    return &this->_AssetResponse;
  }
private:
  std::shared_ptr<IAssetRequest>_pAssetRequest;
  DecryptAssetResponse _AssetResponse;
};

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
DecryptIfNeeded(
    const CesiumAsync::AsyncSystem& asyncSystem,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
  const CesiumAsync::IAssetResponse* pResponse = pCompletedRequest->response();
  if(pResponse && pResponse->headers().find("Encrypted")!=pResponse->headers().end()) {
    return asyncSystem.runInWorkerThread(
      [pCompletedRequest=std::move(
        pCompletedRequest)]()mutable->std::shared_ptr<CesiumAsync::IAssetRequest> {
        return std::make_shared<DecryptAssetRequest>(
          std::move(pCompletedRequest));
      });
  }
  return asyncSystem.createResolvedFuture(std::move(pCompletedRequest));
}

}
EncryptAssetAccessor::EncryptAssetAccessor(
  const std::shared_ptr<IAssetAccessor>& pAssetAccessor)
  :_pAssetAccessor(pAssetAccessor){}

EncryptAssetAccessor::~EncryptAssetAccessor() noexcept {}


CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> EncryptAssetAccessor::get(
  const CesiumAsync::AsyncSystem& asyncSystem,
  const std::string& url,
  const std::vector<THeader>& headers) {
  return this->_pAssetAccessor->get(asyncSystem,url,headers)
  .thenImmediately(
    [asyncSystem](std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
      return DecryptIfNeeded(asyncSystem,std::move(pCompletedRequest));
    });
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> EncryptAssetAccessor::request(
  const CesiumAsync::AsyncSystem& asyncSystem,
  const std::string& verb, const std::string& url,
  const std::vector<THeader>& headers,
  const gsl::span<const std::byte>& contentPayload) {
  return this->_pAssetAccessor->request(asyncSystem,verb,url,headers,contentPayload)
  .thenImmediately([asyncSystem](std::shared_ptr<CesiumAsync::IAssetRequest>&& pCompletedRequest) {
    return DecryptIfNeeded(asyncSystem,std::move(pCompletedRequest));
  });
}

void EncryptAssetAccessor::tick() noexcept {
  _pAssetAccessor->tick();
}

