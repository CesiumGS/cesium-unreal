// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumCustomVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid
    FCesiumCustomVersion::GUID(0xA5DCCA38, 0xDDA34991, 0x98E7B2F2, 0x15E17470);

// Register the custom version with core
FCustomVersionRegistration GRegisterCesiumCustomVersion(
    FCesiumCustomVersion::GUID,
    FCesiumCustomVersion::LatestVersion,
    TEXT("CesiumVer"));
