#pragma once

#include "CesiumAsync/IAssetAccessor.h"

class UnrealAssetAccessor : public CesiumAsync::IAssetAccessor
{
public:
    virtual void requestAsset(
        const CesiumAsync::AsyncSystem* asyncSystem,
        const std::string& url,
        const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
        std::function<void(std::shared_ptr<CesiumAsync::IAssetRequest>)> callback
    ) override;

    virtual void tick() noexcept override;
};
