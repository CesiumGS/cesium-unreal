#pragma once

#include <string>
#include "Camera.h"
#include "ViewUpdateResult.h"

namespace Cesium3DTiles {
    class Tileset;

    class CESIUM3DTILES_API TilesetView {
    public:
        TilesetView(Tileset& tileset, const std::string& name);

        /// <summary>
        /// Updates this view, returning the set of tiles to render in this view.
        /// </summary>
        /// <param name="camera">The updated camera.</param>
        /// <returns>
        /// The set of tiles to render in the updated camera view. This value is only valid until
        /// the next call to <see cref="Cesium3DTilesetView.update" /> or until the view is
        /// destroyed, whichever comes first.
        /// </returns>
        const ViewUpdateResult& update(const Camera& camera);

    private:
        Tileset& _tileset;
        std::string _name;
        ViewUpdateResult _updateResult;
    };

}
