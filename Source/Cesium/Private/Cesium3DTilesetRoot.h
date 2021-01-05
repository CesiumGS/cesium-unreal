// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include <glm/vec3.hpp>
#include <optional>
#include "Cesium3DTilesetRoot.generated.h"


UCLASS()
class UCesium3DTilesetRoot : public USceneComponent
{
	GENERATED_BODY()

public:	
	UCesium3DTilesetRoot();

	/**
	 * @brief Determines if {@link GetCesiumTilesetToUnrealRelativeWorldTransform} has changed.
	 * 
	 * Returns true if the value returned by {@link GetCesiumTilesetToUnrealRelativeWorldTransform} has
	 * changed since the last time {@link MarkTransformUnchanged} was called.
	 */
	bool IsTransformChanged() const { return this->_isDirty; }

	/**
	 * @brief Marks {@link GetCesiumTilesetToUnrealRelativeWorldTransform} unchanged.
	 * 
	 * After calling this function, {@link IsTransformChanged} will return false until the next
	 * time that the transform changes.
	 */
	void MarkTransformUnchanged();

	/**
	 * @brief Recalculates {@link GetCesiumTilesetToUnrealRelativeWorldTransform} and marks it changed.
	 * 
	 * It is not usually necessary to call this method directly.
	 */
	void RecalculateTransform();

	/**
	 * @brief Recalculates {@link GetCesiumTilesetToUnrealRelativeWorldTransform} using the new ellipsoid-centered to
	 * georeferenced transform and marks it changed.
	 * 
	 * @param ellipsoidCenteredToGeoreferencedTransform The updated ellipsoid-centered to georeferenced transform to use 
	 */
	void UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform);

	/**
	 * @brief Gets the transform from the "Cesium Tileset" reference frame to the "Unreal Relative World" reference frame.
	 * 
	 * Gets a matrix that transforms coordinates from the "Cesium Tileset" reference frame (which is _usually_
	 * Earth-centered, Earth-fixed) to Unreal Engine's relative world coordinates (i.e. relative to the world OriginLocation).
	 * 
	 * See {@link reference-frames.md}.
	 * 
	 * This transformation is a function of :
	 *   * The location of the Tileset in "Unreal Absolute World" coordinates.
	 *   * The rotation and scale of the tileset relative to the Unreal World.
	 *   * `UWorld::OriginLocation`
	 *   * The transformation from ellipsoid-centered to georeferenced coordinates, as provided by `CesiumGeoreference`.
	 * 
	 * @param newOriginLocation The updated World `OriginLocation`. If this is `std::nullopt`, the `OriginLocation` is obtained
	 *        directly from the `UWorld`.
	 */
	const glm::dmat4& GetCesiumTilesetToUnrealRelativeWorldTransform() const;

	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

protected:
	virtual void BeginPlay() override;
	virtual bool MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = NULL, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags, ETeleportType Teleport = ETeleportType::None) override;

private:
	void _updateAbsoluteLocation();
	void _updateTilesetToUnrealRelativeWorldTransform();
	void _updateTilesetToUnrealRelativeWorldTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform);

	glm::dvec3 _worldOriginLocation;
	glm::dvec3 _absoluteLocation;
	glm::dmat4 _tilesetToUnrealRelativeWorld;
	bool _isDirty;
};
