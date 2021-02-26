#pragma once

#include "CesiumIonClient/Connection.h"
#include "CesiumIonClient/Profile.h"
#include <optional>

DECLARE_MULTICAST_DELEGATE(FIonConnectionChangedDelegate);

struct IonConnectionDetails {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;
    std::unique_ptr<CesiumAsync::AsyncSystem> pAsyncSystem;
    std::optional<CesiumIonClient::Connection> connection;
    std::optional<CesiumIonClient::Profile> profile;
    std::string token;
    FIonConnectionChangedDelegate ionConnectionChanged;
};
