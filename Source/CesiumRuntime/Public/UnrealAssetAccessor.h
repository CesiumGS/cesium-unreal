// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "Containers/UnrealString.h"
#include "HAL/Platform.h"
#include <cstddef>

class CESIUMRUNTIME_API UnrealAssetAccessor
    : public CesiumAsync::IAssetAccessor {
public:
  UnrealAssetAccessor();

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers)
      override;

  virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
      const gsl::span<const std::byte>& contentPayload) override;

  virtual void tick() noexcept override;

private:
  FString _userAgent;
};
