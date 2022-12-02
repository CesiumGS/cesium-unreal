// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMaterialUserData.h"
#include "Materials/MaterialInstance.h"
#include "Runtime/Launch/Resources/Version.h"

void UCesiumMaterialUserData::PostEditChangeOwner() {
  Super::PostEditChangeOwner();

#if WITH_EDITORONLY_DATA
  this->LayerNames.Empty();

  UMaterialInstance* pMaterial = Cast<UMaterialInstance>(this->GetOuter());
  if (pMaterial) {
    const FStaticParameterSet& parameters = pMaterial->GetStaticParameters();

    const auto& layerParameters = parameters.MaterialLayers;

    this->LayerNames.Reserve(layerParameters.Layers.Num());

    for (int32 i = 0; i < layerParameters.Layers.Num(); ++i) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
      this->LayerNames.Add(layerParameters.GetLayerName(i).ToString());
#else
      this->LayerNames.Add(layerParameters.Layers[i].GetName());
#endif
    }
  }
#endif
}
