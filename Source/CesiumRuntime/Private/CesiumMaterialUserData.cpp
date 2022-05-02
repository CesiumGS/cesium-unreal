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

#if ENGINE_MAJOR_VERSION >= 5
    const FMaterialLayersFunctions& layerParameters = parameters.MaterialLayers;

    this->LayerNames.Reserve(layerParameters.Layers.Num());

    for (int32 i = 0; i < layerParameters.Layers.Num(); ++i) {
      this->LayerNames.Add(layerParameters.GetLayerName(i).ToString());
    }
#else
    const TArray<FStaticMaterialLayersParameter>& layerParameters =
        parameters.MaterialLayersParameters;

    for (const FStaticMaterialLayersParameter& layerParameter :
         layerParameters) {
      if (layerParameter.ParameterInfo.Name != "Cesium")
        continue;

      this->LayerNames.Reserve(layerParameter.Value.LayerNames.Num());
      for (const FText& text : layerParameter.Value.LayerNames) {
        this->LayerNames.Add(text.ToString());
      }
    }
#endif
  }
#endif
}
