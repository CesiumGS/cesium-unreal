#pragma once

#include <list>
#include <memory>
#include "Cesium3DTiles/IAssetAccessor.h"

class UnrealAssetAccessor : public Cesium3DTiles::IAssetAccessor
{
public:
    virtual std::unique_ptr<Cesium3DTiles::IAssetRequest> requestAsset(
        const std::string& url,
        const std::vector<Cesium3DTiles::IAssetAccessor::THeader>& headers
    ) override;
    virtual void tick() override;
};
