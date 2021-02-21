#pragma once

#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"

class UnrealAssetAccessor : public CesiumAsync::IAssetAccessor
{
public:
    virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> requestAsset(
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::string& url,
        const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers
    ) override;

    virtual void tick() noexcept override;
};
