// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "UObject/ObjectMacros.h"

#include "CesiumLoadedTile.generated.h"

namespace CesiumGltf {
struct Model;
}

UINTERFACE()
class UCesiumLoadedTile : public UInterface {
  GENERATED_BODY()
};
class ICesiumLoadedTile {
  GENERATED_BODY()
public:
  virtual const CesiumGltf::Model* GetGltfModel() const = 0;
};
