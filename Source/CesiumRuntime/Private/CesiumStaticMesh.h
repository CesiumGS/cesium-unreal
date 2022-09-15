// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Engine/StaticMesh.h"
#include "CesiumStaticMesh.generated.h"

UCLASS()
class UCesiumStaticMesh : public UStaticMesh {
  GENERATED_UCLASS_BODY()

public:
  bool IsReadyForFinishDestroy() override;
};