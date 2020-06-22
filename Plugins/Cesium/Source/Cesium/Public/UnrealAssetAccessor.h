#pragma once

#include <list>
#include <memory>
#include "IAssetAccessor.h"

class UnrealAssetAccessor : public Cesium3DTiles::IAssetAccessor
{
public:
    virtual std::unique_ptr<Cesium3DTiles::IAssetRequest> requestAsset(const std::string& url) override;
};
