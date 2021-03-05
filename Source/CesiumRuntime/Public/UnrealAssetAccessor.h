// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/AsyncSystem.h"

class CESIUMRUNTIME_API UnrealAssetAccessor : public CesiumAsync::IAssetAccessor
{
public:
    virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> requestAsset(
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::string& url,
        const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers
    ) override;

    virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> post(
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::string& url,
        const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
        const gsl::span<const uint8_t>& contentPayload
    ) override;

    virtual void tick() noexcept override;
};
