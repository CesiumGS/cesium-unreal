// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include <glm/mat4x4.hpp>
#include "ACesium3DTileset.generated.h"

namespace Cesium3DTiles {
	class Tileset;
	class TilesetView;
}

UCLASS()
class CESIUM_API ACesium3DTileset : public AActor
{
	GENERATED_BODY()
	
public:	
	ACesium3DTileset();
	
	/**
	 * The URL of this tileset's "tileset.json" file. If this property is specified, the ion asset ID
	 * and token are ignored.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	FString Url;

	/**
	 * The ID of the Cesium ion asset to use. This property is ignored if the Url is
	 * specified.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	uint32 IonAssetID;

	/**
	 * The access token to use to access the Cesium ion resource.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="IonAssetID"))
	FString IonAccessToken;

	/**
	 * If true, the center of this tileset's bounding volume will be placed at the Unreal world origin,
	 * even if that is not the tileset's origin. This is useful because 3D Tiles tilesets often use
	 * Earth-centered, Earth-fixed coordinates, such that tileset content is in a small bounding
	 * volume 6-7 million meters (the radius of the Earth) away from the coordinate system origin.
	 * If false, the tileset's true coordinates are used.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	bool PlaceTilesetBoundingVolumeCenterAtWorldOrigin = true;

	/**
	 * If true, the tileset is rotated so that the local up at the center of the tileset's bounding
	 * volume is aligned with the usual Unreal Engine up direction, +Z. This is useful because
	 * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates in which the local
	 * up direction depends on where you are on the Earth. If false, the tileset's true rotation
	 * is used.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	bool AlignTilesetUpWithZ = true;

	/**
	 * Pauses level-of-detail and culling updates of this tileset.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Debug")
	bool SuspendUpdate;

	glm::dmat4x4 GetWorldToTilesetTransform() const;
	glm::dmat4x4 GetTilesetToWorldTransform() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void LoadTileset();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	Cesium3DTiles::Tileset* _pTileset;
	Cesium3DTiles::TilesetView* _pTilesetView;
};
