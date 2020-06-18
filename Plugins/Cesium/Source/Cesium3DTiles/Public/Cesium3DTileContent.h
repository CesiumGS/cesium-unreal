#pragma once

class Cesium3DTile;

class Cesium3DTileContent {
public:
    Cesium3DTileContent(const Cesium3DTile& tile);
    virtual ~Cesium3DTileContent();

    void* rendererResource;

private:
    const Cesium3DTile* _pTile;
};
