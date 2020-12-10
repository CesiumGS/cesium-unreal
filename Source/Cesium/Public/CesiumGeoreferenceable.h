// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Cesium3DTiles/BoundingVolume.h"
#include <optional>
#include "CesiumGeoreferenceable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCesiumGeoreferenceable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CESIUM_API ICesiumGeoreferenceable
{
	GENERATED_BODY()

public:
	/**
	 * Determines if the bounding volume of this georeferenceable object is currently available.
	 */
	virtual bool IsBoundingVolumeReady() const = 0;

	/**
	 * Gets the bounding volume of this georeferenceable object. If the bounding volume is not
	 * yet ready or if the object has no bounding volume, nullopt will be returned. Use
	 * {@see IsBoundingVolumeReady} to determine which of these is the case.
	 */
	virtual std::optional<Cesium3DTiles::BoundingVolume> GetBoundingVolume() const = 0;

	/**
	 * Updates this object with a new transformation from the ellipsoid-centered coordinate system to
	 * the local coordinates of the georeferenced origin.
	 */
	virtual void UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedOriginTransform) = 0;
};
