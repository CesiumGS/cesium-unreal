#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "TilesetExternals.h"
#include "TilesetView.h"
#include "Tile.h"
#include "IAssetRequest.h"

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
        /// Creates a new view of this tileset. Views share a common cache of tiles but do independent
        /// culling and level-of-detail.
        /// </summary>
        /// <param name="name">The name of the view. This is just for debugging and need not be unique.</param>
        /// <returns>The new view</returns>
        TilesetView& createView(const std::string& name);

        /// <summary>
        /// Destroys a view. The view must not be accessed after this method is called.
        /// </summary>
        /// <param name="view">The view to destroy.</param>
        void destroyView(TilesetView& view);

        /// <summary>
        /// Gets the currently-active views of this tileset.
        /// </summary>
        /// <returns>The views</returns>
        const std::vector<std::unique_ptr<TilesetView>>& getViews() const { return this->_views; }

        /// <summary>
        /// Gets the root tile of this tileset, or nullptr if there is currently no root tile.
        /// </summary>
        Tile* getRootTile() { return this->_pRootTile.data(); }
        const Tile* getRootTile() const { return this->_pRootTile.data(); }

    protected:
        void ionResponseReceived(IAssetRequest* pRequest);
        void tilesetJsonResponseReceived(IAssetRequest* pRequest);
        void createTile(VectorReference<Tile>& tile, const nlohmann::json& tileJson, const std::string& baseUrl);

    private:
        TilesetExternals _externals;
        std::vector<std::unique_ptr<TilesetView>> _views;

        std::optional<std::string> _url;
        std::optional<uint32_t> _ionAssetID;
        std::optional<std::string> _ionAccessToken;

        std::unique_ptr<IAssetRequest> _pTilesetRequest;

        std::vector<Tile> _tiles;
        VectorReference<Tile> _pRootTile;

        Tileset(const Tileset& rhs) = delete;
        Tileset& operator=(const Tileset& rhs) = delete;
    };

} // namespace Cesium::ThreeDTiles