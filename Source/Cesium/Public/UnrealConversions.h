#pragma once

#include <string>
#include "CoreMinimal.h"
#include "glm/vec3.hpp"

CESIUM_API FString utf8_to_wstr(const std::string& utf8);
CESIUM_API std::string wstr_to_utf8(const FString& utf16);

inline glm::dvec3 cesiumVectorToUnrealVector(const glm::dvec3& cesiumVector)
{
	return glm::dvec3(cesiumVector.x, cesiumVector.z, cesiumVector.y);
}

inline FVector cesiumVectorToUnrealFVector(const glm::dvec3& cesiumVector)
{
	return FVector(cesiumVector.x, cesiumVector.z, cesiumVector.y);
}

inline glm::dvec3 cesiumFVectorToUnrealVector(const FVector& cesiumVector)
{
	return glm::dvec3(cesiumVector.X, cesiumVector.Z, cesiumVector.Y);
}

inline FVector cesiumFVectorToUnrealFVector(const FVector& cesiumVector)
{
	return FVector(cesiumVector.X, cesiumVector.Z, cesiumVector.Y);
}

inline glm::dvec3 unrealVectorToCesiumVector(const glm::dvec3& unrealVector)
{
	return glm::dvec3(unrealVector.x, unrealVector.z, unrealVector.y);
}

inline FVector unrealVectorToCesiumFVector(const glm::dvec3& unrealVector)
{
	return FVector(unrealVector.x, unrealVector.z, unrealVector.y);
}

inline glm::dvec3 unrealFVectorToCesiumVector(const FVector& unrealVector)
{
	return glm::dvec3(unrealVector.X, unrealVector.Z, unrealVector.Y);
}

inline FVector unrealFVectorToCesiumFVector(const FVector& unrealVector)
{
	return FVector(unrealVector.X, unrealVector.Z, unrealVector.Y);
}
