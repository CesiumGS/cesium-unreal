#pragma once

#include "CesiumAsync/IAssetAccessor.h"

class UnrealAssetAccessor : public CesiumAsync::IAssetAccessor
{
public:
    virtual std::unique_ptr<CesiumAsync::IAssetRequest> requestAsset(
        const std::string& url,
        const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers
    ) override;
    virtual void tick() noexcept override;
};
