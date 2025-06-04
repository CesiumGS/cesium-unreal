// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "UObject/ObjectMacros.h"

#include "CesiumLoadedTile.generated.h"

UINTERFACE()
class UCesiumLoadedTile : public UInterface {
  GENERATED_BODY()
};
class ICesiumLoadedTile {
  GENERATED_BODY()
public:
  virtual std::optional<int32> GetGltfModelVersion() const = 0;
};
