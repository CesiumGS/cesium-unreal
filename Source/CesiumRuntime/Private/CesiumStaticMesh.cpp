// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumStaticMesh.h"

UCesiumStaticMesh::UCesiumStaticMesh(
    const FObjectInitializer& ObjectInitializer)
    : UStaticMesh(ObjectInitializer) {}

bool UCesiumStaticMesh::IsReadyForFinishDestroy() {
#if WITH_EDITOR
  // We're being garbage collected and might still have async tasks pending
  if (!TryCancelAsyncTasks()) {
    return false;
  }
#endif

  // Tick base class to make progress on the streaming before calling
  // HasPendingInitOrStreaming().
  if (!UStreamableRenderAsset::IsReadyForFinishDestroy()) {
    return false;
  }

  // Match BeginDestroy() by checking for HasPendingInitOrStreaming().
  if (HasPendingInitOrStreaming()) {
    return false;
  }
  if (bRenderingResourcesInitialized) {
    ReleaseResources();
  }
  return ReleaseResourcesFence.IsFenceComplete();
}