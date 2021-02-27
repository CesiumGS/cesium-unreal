// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

namespace Chaos
{
	void CesiumCleanTriMeshes(TArray<FVector>& InOutVertices, TArray<int32>& InOutIndices, TArray<int32>* OutOptFaceRemap);
}
