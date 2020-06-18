#pragma once

#include <vector>

class Cesium3DTile;

class ViewUpdateResult {
public:
    std::vector<Cesium3DTile*> tilesToRenderThisFrame;
    
    std::vector<Cesium3DTile*> newTilesToRenderThisFrame;
    std::vector<Cesium3DTile*> tilesToNoLongerRenderThisFrame;

    uint32_t tilesLoading;
};
