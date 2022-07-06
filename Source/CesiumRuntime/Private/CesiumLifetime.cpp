// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumLifetime.h"
#include "Async/Async.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/Object.h"
#include <algorithm>

/*static*/ TArray<TWeakObjectPtr<UObject>> CesiumLifetime::_pending;
/*static*/ TArray<TWeakObjectPtr<UObject>> CesiumLifetime::_nextPending;
/*static*/ bool CesiumLifetime::_isScheduled = false;

/*static*/ void CesiumLifetime::destroy(UObject* pObject) {
  if (!runDestruction(pObject)) {
    // Object is not finished being destroyed, so add it to the pending list.
    addToPending(pObject);
  }
}

/*static*/ void
CesiumLifetime::destroyComponentRecursively(USceneComponent* pComponent) {
  UE_LOG(
      LogCesium,
      VeryVerbose,
      TEXT("Destroying scene component recursively"));

  if (!pComponent) {
    return;
  }

  if (pComponent->IsRegistered()) {
    pComponent->UnregisterComponent();
  }

  TArray<USceneComponent*> children = pComponent->GetAttachChildren();
  for (USceneComponent* pChild : children) {
    destroyComponentRecursively(pChild);
  }

  pComponent->DestroyPhysicsState();
  pComponent->DestroyComponent();
  pComponent->ConditionalBeginDestroy();

  UE_LOG(LogCesium, VeryVerbose, TEXT("Destroying scene component done"));
}

/*static*/ bool CesiumLifetime::runDestruction(UObject* pObject) {
  if (!pObject) {
    return true;
  }

#if ENGINE_MAJOR_VERSION >= 5
  pObject->MarkAsGarbage();
#else
  if (!pObject->IsPendingKill()) {
    pObject->MarkPendingKill();
  }
#endif

  if (pObject->HasAnyFlags(RF_FinishDestroyed)) {
    // Already done being destroyed.
    return true;
  }

  if (!pObject->HasAnyFlags(RF_BeginDestroyed)) {
    pObject->ConditionalBeginDestroy();
  }

  if (!pObject->HasAnyFlags(RF_FinishDestroyed) &&
      pObject->IsReadyForFinishDestroy()) {
    // Don't actually call ConditionalFinishDestroy here, because if we do the
    // UE garbage collector will freak out that it's already been called. The
    // IsReadyForFinishDestroy call is important, though. In some objects,
    // calling that actually continues the async destruction!
    finalizeDestroy(pObject);
    return true;
  }

  return false;
}

/*static*/ void CesiumLifetime::addToPending(UObject* pObject) {
  _pending.Add(pObject);
  if (!_isScheduled) {
    _isScheduled = true;
    AsyncTask(ENamedThreads::GameThread, []() {
      CesiumLifetime::processPending();
    });
  }
}

/*static*/ void CesiumLifetime::processPending() {
  _isScheduled = false;

  std::swap(_nextPending, _pending);
  _pending.Empty();

  for (TWeakObjectPtr<UObject> pObject : _nextPending) {
    destroy(pObject.Get(true));
  }
}

/*static*/ void CesiumLifetime::finalizeDestroy(UObject* pObject) {
  // The freeing/clearing/destroying done here is normally done in these
  // objects' FinishDestroy method, but unfortunately we can't call that
  // directly without confusing the garbage collector if and when it _does_
  // run. So instead we manually release some critical resources here.

  UTexture2D* pTexture2D = Cast<UTexture2D>(pObject);
  if (pTexture2D) {
#if ENGINE_MAJOR_VERSION >= 5
    FTexturePlatformData* pPlatformData = pTexture2D->GetPlatformData();
    pTexture2D->SetPlatformData(nullptr);
    delete pPlatformData;
#else
    delete pTexture2D->PlatformData;
    pTexture2D->PlatformData = nullptr;
#endif
  }

  UStaticMesh* pMesh = Cast<UStaticMesh>(pObject);
  if (pMesh) {
#if ENGINE_MAJOR_VERSION >= 5 ||                                               \
    (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27)
    pMesh->SetRenderData(nullptr);
#else
    pMesh->RenderData.Reset();
#endif
  }

  UBodySetup* pBodySetup = Cast<UBodySetup>(pObject);
  if (pBodySetup) {
    pBodySetup->UVInfo.IndexBuffer.Empty();
    pBodySetup->UVInfo.VertPositions.Empty();
    pBodySetup->UVInfo.VertUVs.Empty();
    pBodySetup->FaceRemap.Empty();
    pBodySetup->ClearPhysicsMeshes();
  }
}
