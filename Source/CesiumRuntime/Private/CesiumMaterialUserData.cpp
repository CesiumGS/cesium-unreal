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

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
    const TArray<FText>& layerNames =
        parameters.EditorOnly.MaterialLayers.LayerNames;

    this->LayerNames.Reserve(layerNames.Num());

    for (int32 i = 0; i < layerNames.Num(); ++i) {
      this->LayerNames.Add(layerNames[i].ToString());
    }
#else
    const auto& layerParameters = parameters.MaterialLayers;

    this->LayerNames.Reserve(layerParameters.Layers.Num());

    for (int32 i = 0; i < layerParameters.Layers.Num(); ++i) {
      this->LayerNames.Add(layerParameters.GetLayerName(i).ToString());
    }
#endif
  }
#endif
}
