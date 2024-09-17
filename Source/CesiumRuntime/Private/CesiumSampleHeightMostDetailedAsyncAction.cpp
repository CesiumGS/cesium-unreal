// Copyright 2020-2024 CesiumGS, Inc. and Contributors
#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "Cesium3DTileset.h"
#include "CesiumRuntime.h"

/*static*/ UCesiumSampleHeightMostDetailedAsyncAction*
UCesiumSampleHeightMostDetailedAsyncAction::SampleHeightMostDetailed(
    ACesium3DTileset* Tileset,
    const TArray<FVector>& LongitudeLatitudeHeightArray) {
  UCesiumSampleHeightMostDetailedAsyncAction* pAsyncAction =
      NewObject<UCesiumSampleHeightMostDetailedAsyncAction>();
  pAsyncAction->_pTileset = Tileset;
  pAsyncAction->_longitudeLatitudeHeightArray = LongitudeLatitudeHeightArray;

  return pAsyncAction;
}

void UCesiumSampleHeightMostDetailedAsyncAction::Activate() {
  this->RegisterWithGameInstance(this->_pTileset);

  this->_pTileset->SampleHeightMostDetailed(
      this->_longitudeLatitudeHeightArray,
      FCesiumSampleHeightMostDetailedCallback::CreateUObject(
          this,
          &UCesiumSampleHeightMostDetailedAsyncAction::RaiseOnHeightsSampled));
}

void UCesiumSampleHeightMostDetailedAsyncAction::RaiseOnHeightsSampled(
    ACesium3DTileset* Tileset,
    const TArray<FCesiumSampleHeightResult>& Result,
    const TArray<FString>& Warnings) {
  this->OnHeightsSampled.Broadcast(Result, Warnings);
  this->SetReadyToDestroy();
}
