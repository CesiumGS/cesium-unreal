#pragma once

#include <string>
#include <map>
#include <vector>
#include <gsl/span>
#include <gsl/string_span>

namespace Cesium3DTiles {

    /// <summary>
    /// A completed response for a 3D Tiles asset.
    /// </summary>
    class IAssetResponse {
    public:
        virtual ~IAssetResponse() = default;

        virtual uint16_t statusCode() = 0;
        //virtual const std::map<std::string, std::string>& headers() = 0;
        virtual gsl::span<const uint8_t> data() = 0;
    };

}
