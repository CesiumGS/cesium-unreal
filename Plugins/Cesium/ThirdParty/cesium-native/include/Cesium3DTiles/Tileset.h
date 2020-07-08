#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <atomic>
#include "TilesetExternals.h"
#include "Tile.h"
#include "IAssetRequest.h"
#include "ViewUpdateResult.h"
#include "Camera.h"

#pragma warning(push)
#pragma warning(disable: 4946)
#include "json.hpp"
#pragma warning(pop)

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Tileset {
    public:
        /// <summary>
        /// Initializes a new instance with a given tileset.json URL.
        /// </summary>
        /// <param name="externals">The external interfaces to use.</param>
        /// <param name="url">The URL of the tileset.json.</param>
        Tileset(const TilesetExternals& externals, const std::string& url);

        /// <summary>
        /// Initializes a new instance with given asset ID on <a href="https://cesium.com/ion/">Cesium ion</a>.
        /// </summary>
        /// <param name="externals">The external interfaces to use.</param>
        /// <param name="ionAssetID">The ID of the Cesium ion asset to use.</param>
        /// <param name="ionAccessToken">The Cesium ion access token authorizing access to the asset.</param>
        Tileset(const TilesetExternals& externals, uint32_t ionAssetID, const std::string& ionAccessToken);

        /// <summary>
        /// Gets the URL that was used to construct this tileset. If the tileset references a Cesium ion asset,
        /// this property will not have a value.
        /// </summary>
        std::optional<std::string> getUrl() { return this->_url; }

        /// <summary>
        /// Gets the Cesium ion asset ID of this tileset. If the tileset references a URL, this property
        /// will not have a value.
        /// </summary>
        std::optional<uint32_t> getIonAssetID() { return this->_ionAssetID; }

        /// <summary>
        /// Gets the Cesium ion access token to use to access this tileset. If the tileset references a URL, this
        /// property will not have a value.
        /// </summary>
        std::optional<std::string> getIonAccessToken() { return this->_ionAccessToken; }

        /// <summary>
        /// Gets the external interfaces used by this tileset.
        /// </summary>
        TilesetExternals& externals() { return this->_externals; }
        const TilesetExternals& getExternals() const { return this->_externals; }

        /// <summary>
        /// Gets the root tile of this tileset, or nullptr if there is currently no root tile.
        /// </summary>
        Tile* getRootTile() { return this->_pRootTile.data(); }
        const Tile* getRootTile() const { return this->_pRootTile.data(); }

        /// <summary>
        /// Updates this view, returning the set of tiles to render in this view. This method _must_ be called
        /// in between calls to <see cref="Tileset::beginFrame" /> and <see cref="Tileset::endFrame" />.
        /// It is not necessary to call it on every render frame.
        /// </summary>
        /// <param name="camera">The updated camera.</param>
        /// <returns>
        /// The set of tiles to render in the updated camera view. This value is only valid until
        /// the next call to <see cref="Cesium3DTilesetView.update" /> or until the view is
        /// destroyed, whichever comes first.
        /// </returns>
        const ViewUpdateResult& updateView(const Camera& camera);

        /// <summary>
        /// Gets the current render frame number when called from inside an <see cref="updateView" />.
        /// When not currently updating, this method returns <code>nullopt</code>.
        /// </summary>
        std::optional<uint32_t> getCurrentFrameNumber() const { return this->_currentFrameNumber; }

        /// <summary>
        /// Gets the previous render frame number. In other words, it is the frame number the last
        /// time <see cref="updateView" /> was called. Prior to the first <see cref="updateView" />,
        /// this method returns 0.
        /// </summary>
        uint32_t getPreviousFrameNumber() const { return this->_previousFrameNumber; }

        /// <summary>
        /// Notifies the tileset that the given tile has finished loading and is ready to render.
        /// This method may be called from any thread.
        /// </summary>
        void notifyTileDoneLoading(Tile* pTile);

    protected:
        void ionResponseReceived(IAssetRequest* pRequest);
        void tilesetJsonResponseReceived(IAssetRequest* pRequest);
        void createTile(VectorReference<Tile>& tile, const nlohmann::json& tileJson, const std::string& baseUrl);
        void _visitTile(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, double maximumScreenSpaceError, Tile& tile, ViewUpdateResult& result);
        void _processLoadQueue();

    private:
        TilesetExternals _externals;

        std::optional<std::string> _url;
        std::optional<uint32_t> _ionAssetID;
        std::optional<std::string> _ionAccessToken;

        std::unique_ptr<IAssetRequest> _pTilesetRequest;

        std::vector<Tile> _tiles;
        VectorReference<Tile> _pRootTile;

        std::optional<uint32_t> _currentFrameNumber;
        uint32_t _previousFrameNumber;
        ViewUpdateResult _updateResult;

        std::vector<Tile*> _loadQueue;
        std::atomic<uint32_t> _loadsInProgress;

        Tileset(const Tileset& rhs) = delete;
        Tileset& operator=(const Tileset& rhs) = delete;
    };

} // namespace Cesium::ThreeDTiles