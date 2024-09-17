// Copyright 2020-2024 CesiumGS, Inc. and Contributors
#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "Cesium3DTileset.h"
#include "CesiumRuntime.h"

namespace {

void startTask(
    UCesiumSampleHeightMostDetailedAsyncAction* pAsyncAction,
    Cesium3DTilesSelection::Tileset* pTileset,
    TArray<FVector2D>&& longitudesAndLatitudes) {
  if (pTileset == nullptr) {
  }
}

} // namespace

/*static*/ UCesiumSampleHeightMostDetailedAsyncAction*
UCesiumSampleHeightMostDetailedAsyncAction::SampleHeightMostDetailed(
    ACesium3DTileset* Tileset,
    const TArray<FVector2D>& LongitudesAndLatitudes) {
  UCesiumSampleHeightMostDetailedAsyncAction* pAsyncAction =
      NewObject<UCesiumSampleHeightMostDetailedAsyncAction>();

  pAsyncAction->RegisterWithGameInstance(Tileset);

  CesiumAsync::Future<Cesium3DTilesSelection::Tileset*> futureNativeTileset =
      Tileset->GetTileset()
          ? getAsyncSystem().createResolvedFuture(Tileset->GetTileset())
          : getAsyncSystem().runInMainThread(
                [Tileset]() { return Tileset->GetTileset(); });

  std::vector<CesiumGeospatial::Cartographic> positions;
  positions.reserve(LongitudesAndLatitudes.Num());

  for (const FVector2D& position : LongitudesAndLatitudes) {
    positions.emplace_back(CesiumGeospatial::Cartographic::fromDegrees(
        position.X,
        position.Y,
        0.0));
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
            TArray<FVector> positions;
            positions.Reserve(result.positions.size());

            for (const CesiumGeospatial::Cartographic& position :
                 result.positions) {
              positions.Emplace(FVector(
                  CesiumUtility::Math::radiansToDegrees(position.longitude),
                  CesiumUtility::Math::radiansToDegrees(position.latitude),
                  position.height));
            }

            TArray<FString> warnings;
            warnings.Reserve(result.warnings.size());

            for (const std::string& warning : result.warnings) {
              warnings.Emplace(UTF8_TO_TCHAR(warning.c_str()));
            }

            pAsyncAction->OnFinished.Broadcast(positions, warnings);
            pAsyncAction->SetReadyToDestroy();
          });

  return pAsyncAction;
}
