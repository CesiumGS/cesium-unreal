// Copyright Epic Games, Inc. All Rights Reserved.
// Copied from C:\Program Files\Epic Games\UE_4.26\Engine\Source\Runtime\Engine\Private\PhysicsEngine\Experimental\ChaosDerivedDataUtil.h

#pragma once

#include "CoreMinimal.h"

namespace Chaos {
void CesiumCleanTriMeshes(
    TArray<FVector>& InOutVertices,
    TArray<int32>& InOutIndices,
    TArray<int32>* OutOptFaceRemap);
}
