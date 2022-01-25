// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMaterialUserData.h"
#include "Materials/MaterialInstance.h"

void UCesiumMaterialUserData::PostEditChangeOwner() {
  Super::PostEditChangeOwner();

#if WITH_EDITORONLY_DATA
  this->LayerNames.Empty();

  UMaterialInstance* pMaterial = Cast<UMaterialInstance>(this->GetOuter());
  if (pMaterial) {
    const FStaticParameterSet& parameters = pMaterial->GetStaticParameters();
    const FMaterialLayersFunctions& layerParameters = parameters.MaterialLayers;

    this->LayerNames.Reserve(layerParameters.Layers.Num());

    for (int32 i = 0; i < layerParameters.Layers.Num(); ++i) {
      this->LayerNames.Add(layerParameters.GetLayerName(i).ToString());
    }
  }
#endif
}
