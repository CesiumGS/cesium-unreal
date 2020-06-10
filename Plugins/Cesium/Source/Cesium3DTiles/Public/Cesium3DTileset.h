#pragma once

#include <string>
#include <optional>

class INetworkRequest;

class INetwork {
public:
    virtual INetworkRequest* createRequest(const std::string& url);
    virtual ~INetwork() = 0 {}
};

struct Cesium3DTilesetOptions {
    INetwork* pNetwork;

    std::optional<std::string> url;
    std::optional<uint32_t> ionAssetID;
    std::optional<std::string> ionAccessToken;


};

class Cesium3DTileset {
public:
    Cesium3DTileset(const Cesium3DTilesetOptions& options);

private:

};
