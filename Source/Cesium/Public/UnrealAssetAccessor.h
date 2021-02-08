#pragma once

#include "CesiumAsync/IAssetAccessor.h"

class CESIUM_API UnrealAssetAccessor : public CesiumAsync::IAssetAccessor
{
public:
    virtual std::unique_ptr<CesiumAsync::IAssetRequest> requestAsset(
        const std::string& url,
        const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers
    ) override;

    virtual std::unique_ptr<CesiumAsync::IAssetRequest> post(
        const std::string& url,
        const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
        const gsl::span<const uint8_t>& contentPayload
    ) override;

    virtual void tick() noexcept override;
};
