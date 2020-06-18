#pragma once

#include <list>
#include <memory>
#include "IAssetAccessor.h"

class UnrealAssetAccessor : public IAssetAccessor
{
public:
    virtual std::unique_ptr<IAssetRequest> requestAsset(const std::string& url) override;
};
