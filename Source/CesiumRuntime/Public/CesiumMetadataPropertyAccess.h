// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Containers/UnrealString.h"

struct FCesiumPrimitiveFeatures;
struct FCesiumModelMetadata;
struct FCesiumPropertyTableProperty;

class CESIUMRUNTIME_API FCesiumMetadataPropertyAccess {

public:
  /**
   * Retrieves a property by name.
   * If the specified feature ID set does not exist or if the property table
   * does not contain a property with that name, this function returns nullptr.
   */
  static const FCesiumPropertyTableProperty* FindValidProperty(
      const FCesiumPrimitiveFeatures& Features,
      const FCesiumModelMetadata& Metadata,
      const FString& PropertyName,
      int64 FeatureIDSetIndex = 0);
};
