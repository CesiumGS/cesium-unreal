// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include <CesiumGltf/Model.h>

#include <gsl/span>

#include <memory>
#include <optional>
#include <vector>

namespace CesiumDerivedDataCache {
struct DerivedDataResult {
  CesiumGltf::Model model;
  std::vector<gsl::span<const std::byte>> cookedPhysicsMeshViews;
};

struct DerivedDataToCache {
  const CesiumGltf::Model* pModel;
  std::vector<gsl::span<const std::byte>> cookedPhysicsMeshViews;
};

std::optional<DerivedDataResult>
deserialize(const gsl::span<const std::byte>& cache);
std::vector<std::byte> serialize(const DerivedDataToCache& derivedData);
} // namespace CesiumDerivedDataCache
