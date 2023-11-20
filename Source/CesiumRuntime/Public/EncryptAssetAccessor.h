#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetRequest.h"
class CESIUMRUNTIME_API EncryptAssetAccessor
    : public CesiumAsync::IAssetAccessor {
public:
  EncryptAssetAccessor(const std::shared_ptr<IAssetAccessor>& pAssetAccessor);

  virtual ~EncryptAssetAccessor() noexcept override;

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers) override;

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers,
      const gsl::span<const std::byte>& contentPayload) override;
  /** @copydoc IAssetAccessor::tick */
  virtual void tick() noexcept override;

private:
  std::shared_ptr<IAssetAccessor> _pAssetAccessor;

};
