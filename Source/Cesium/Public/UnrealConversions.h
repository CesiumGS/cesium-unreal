#pragma once

#include <string>
#include "CoreMinimal.h"
#include "glm/vec3.hpp"

FString utf8_to_wstr(const std::string& utf8);
std::string wstr_to_utf8(const FString& utf16);

inline glm::dvec3 cesiumVectorToUnrealVector(const glm::dvec3& gltfVector)
{
	return glm::dvec3(gltfVector.x, gltfVector.z, gltfVector.y);
}

inline FVector cesiumVectorToUnrealFVector(const glm::dvec3& gltfVector)
{
	return FVector(gltfVector.x, gltfVector.z, gltfVector.y);
}

inline glm::dvec3 cesiumFVectorToUnrealVector(const FVector& gltfVector)
{
	return glm::dvec3(gltfVector.X, gltfVector.Z, gltfVector.Y);
}

inline FVector cesiumFVectorToUnrealFVector(const FVector& gltfVector)
{
	return FVector(gltfVector.X, gltfVector.Z, gltfVector.Y);
}

inline glm::dvec3 unrealVectorToCesiumVector(const glm::dvec3& gltfVector)
{
	return glm::dvec3(gltfVector.x, gltfVector.z, gltfVector.y);
}

inline FVector unrealVectorToCesiumFVector(const glm::dvec3& gltfVector)
{
	return FVector(gltfVector.x, gltfVector.z, gltfVector.y);
}

inline glm::dvec3 unrealFVectorToCesiumVector(const FVector& gltfVector)
{
	return glm::dvec3(gltfVector.X, gltfVector.Z, gltfVector.Y);
}

inline FVector unrealFVectorToCesiumFVector(const FVector& gltfVector)
{
	return FVector(gltfVector.X, gltfVector.Z, gltfVector.Y);
}
