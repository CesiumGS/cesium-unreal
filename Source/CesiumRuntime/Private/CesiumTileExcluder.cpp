// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumTileExcluder.h"
#include "Cesium3DTileset.h"
#include "CesiumLifetime.h"
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
  ACesium3DTileset* CesiumTileset = this->GetOwner<ACesium3DTileset>();
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

  CesiumTile = NewObject<UCesiumTile>(this);
  CesiumTile->SetVisibility(false);
  CesiumTile->SetMobility(EComponentMobility::Movable);
  CesiumTile->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  CesiumTile->SetupAttachment(CesiumTileset->GetRootComponent());
  CesiumTile->RegisterComponent();

  auto pAdapter = std::make_shared<CesiumTileExcluderAdapter>(
      TWeakObjectPtr<UCesiumTileExcluder>(this),
      CesiumTileset->ResolveGeoreference(),
      CesiumTile);
  pExcluderAdapter = pAdapter.get();
  excluders.push_back(std::move(pAdapter));
}

void UCesiumTileExcluder::RemoveFromTileset() {
  ACesium3DTileset* CesiumTileset = this->GetOwner<ACesium3DTileset>();
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

  CesiumLifetime::destroyComponentRecursively(CesiumTile);
}

void UCesiumTileExcluder::Refresh() {
  this->RemoveFromTileset();
  this->AddToTileset();
}

bool UCesiumTileExcluder::ShouldExclude_Implementation(
    const UCesiumTile* TileObject) {
  return false;
}

void UCesiumTileExcluder::Activate(bool bReset) {
  Super::Activate(bReset);
  this->AddToTileset();
}

void UCesiumTileExcluder::Deactivate() {
  Super::Deactivate();
  this->RemoveFromTileset();
}

void UCesiumTileExcluder::OnComponentDestroyed(bool bDestroyingHierarchy) {
  this->RemoveFromTileset();
  Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
// Called when properties are changed in the editor
void UCesiumTileExcluder::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  this->RemoveFromTileset();
  this->AddToTileset();
}
#endif
