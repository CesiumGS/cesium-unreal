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

  pAsyncAction->RegisterWithGameInstance(Tileset);

  CesiumAsync::Future<Cesium3DTilesSelection::Tileset*> futureNativeTileset =
      Tileset->GetTileset()
          ? getAsyncSystem().createResolvedFuture(Tileset->GetTileset())
          : getAsyncSystem().runInMainThread(
                [Tileset]() { return Tileset->GetTileset(); });

  std::vector<CesiumGeospatial::Cartographic> positions;
  positions.reserve(LongitudeLatitudeHeightArray.Num());

  for (const FVector& position : LongitudeLatitudeHeightArray) {
    positions.emplace_back(CesiumGeospatial::Cartographic::fromDegrees(
        position.X,
        position.Y,
        position.Z));
  }

  std::move(futureNativeTileset)
      .thenImmediately([positions = std::move(positions)](
                           Cesium3DTilesSelection::Tileset* pTileset) {
        if (pTileset) {
          return pTileset->sampleHeightMostDetailed(positions);
        } else {
          return getAsyncSystem().createResolvedFuture(
              Cesium3DTilesSelection::SampleHeightResult{
                  {},
                  {},
                  {"Could not sample heights from tileset because it has not been created."}});
        }
      })
      .thenInMainThread(
          [pAsyncAction](Cesium3DTilesSelection::SampleHeightResult&& result) {
            check(result.positions.size() == result.heightSampled.size());

            // This should do nothing, but will prevent undefined behavior if
            // the array sizes are unexpectedly different.
            result.heightSampled.resize(result.positions.size(), false);

            TArray<FSampleHeightResult> ue;
            ue.Reserve(result.positions.size());

            for (size_t i = 0; i < result.positions.size(); ++i) {
              const CesiumGeospatial::Cartographic& position =
                  result.positions[i];

              FSampleHeightResult uePosition;
              uePosition.LongitudeLatitudeHeight = FVector(
                  CesiumUtility::Math::radiansToDegrees(position.longitude),
                  CesiumUtility::Math::radiansToDegrees(position.latitude),
                  position.height);
              uePosition.HeightSampled = result.heightSampled[i];

              ue.Emplace(std::move(uePosition));
            }

            TArray<FString> warnings;
            warnings.Reserve(result.warnings.size());

            for (const std::string& warning : result.warnings) {
              warnings.Emplace(UTF8_TO_TCHAR(warning.c_str()));
            }

            pAsyncAction->OnHeightsSampled.Broadcast(ue, warnings);
            pAsyncAction->SetReadyToDestroy();
          });

  return pAsyncAction;
}
