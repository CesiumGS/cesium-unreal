#pragma once

#include "CesiumIonClient/CesiumIonConnection.h"
#include "CesiumIonClient/CesiumIonProfile.h"
#include <optional>

struct IonConnectionDetails {
    std::unique_ptr<CesiumAsync::AsyncSystem> pAsyncSystem;
    std::optional<CesiumIonClient::CesiumIonConnection> connection;
    std::optional<CesiumIonClient::CesiumIonProfile> profile;
};
