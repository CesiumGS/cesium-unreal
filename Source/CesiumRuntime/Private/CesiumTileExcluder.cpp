#include "CesiumTileExcluder.h"
#include "CesiumTileExcluderAdapter.h"

using namespace Cesium3DTilesSelection;

namespace {
auto findExistingExcluder(
    const std::vector<std::shared_ptr<ITileExcluder>>& excluders,
    const CesiumTileExcluderAdapter& excluder) {
  return std::find_if(
      excluders.begin(),
      excluders.end(),
      [&excluder](const std::shared_ptr<ITileExcluder>& pCandidate) {
        return pCandidate.get() == &excluder;
      });
}
} // namespace

UCesiumTileExcluder::UCesiumTileExcluder(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  PrimaryComponentTick.bCanEverTick = false;
  bAutoActivate = true;
}

void UCesiumTileExcluder::AddToTileset() {
  CesiumTileset = this->GetOwner<ACesium3DTileset>();
  if (!CesiumTileset)
    return;
  Tileset* pTileset = CesiumTileset->GetTileset();
  if (!pTileset)
    return;

  std::vector<std::shared_ptr<ITileExcluder>>& excluders =
      pTileset->getOptions().excluders;

  auto it = findExistingExcluder(excluders, *this->pExcluderAdapter);
  if (it != excluders.end())
    return;

  CesiumTile = NewObject<UCesiumTile>();
  auto pAdapter = std::make_shared<CesiumTileExcluderAdapter>(
      TWeakObjectPtr<UCesiumTileExcluder>(this),
      CesiumTileset,
      CesiumTile);
  pExcluderAdapter = pAdapter.get();
  excluders.push_back(std::move(pAdapter));
}

void UCesiumTileExcluder::RemoveFromTileset() {
  CesiumTileset = this->GetOwner<ACesium3DTileset>();
  if (!CesiumTileset)
    return;
  Tileset* pTileset = CesiumTileset->GetTileset();
  if (!pTileset)
    return;

  std::vector<std::shared_ptr<ITileExcluder>>& excluders =
      pTileset->getOptions().excluders;

  auto it = findExistingExcluder(excluders, *pExcluderAdapter);
  if (it != excluders.end()) {
    excluders.erase(it);
  }
}

bool UCesiumTileExcluder::ShouldExclude_Implementation(
    const UCesiumTile* TileObject) {
  return false;
}
