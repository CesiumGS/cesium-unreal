// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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

    const TArray<FText>& layerNames =
        parameters.EditorOnly.MaterialLayers.LayerNames;

    this->LayerNames.Reserve(layerNames.Num());

    for (int32 i = 0; i < layerNames.Num(); ++i) {
      this->LayerNames.Add(layerNames[i].ToString());
    }
  }
#endif
}
