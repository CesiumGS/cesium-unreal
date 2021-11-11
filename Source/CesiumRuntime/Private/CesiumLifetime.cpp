#include "CesiumLifetime.h"
#include "Async/Async.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/Object.h"

/*static*/ TArray<TWeakObjectPtr<UObject>> CesiumLifetime::_pending;
/*static*/ TArray<TWeakObjectPtr<UObject>> CesiumLifetime::_nextPending;
/*static*/ bool CesiumLifetime::_isScheduled = false;

/*static*/ void CesiumLifetime::destroy(UObject* pObject) {
  if (!runDestruction(pObject)) {
    // Object is not finished being destroyed, so add it to the pending list.
    addToPending(pObject);
  }
}

/*static*/ bool CesiumLifetime::runDestruction(UObject* pObject) {
  if (!pObject) {
    return true;
  }

  if (!pObject->IsPendingKill()) {
    pObject->MarkPendingKill();
  }

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
  UTexture2D* pTexture2D = Cast<UTexture2D>(pObject);
  if (pTexture2D) {
    delete pTexture2D->PlatformData;
    pTexture2D->PlatformData = nullptr;
  }

  UStaticMesh* pMesh = Cast<UStaticMesh>(pObject);
  if (pMesh) {
    pMesh->RenderData.Reset();
  }

  UBodySetup* pBodySetup = Cast<UBodySetup>(pObject);
  if (pBodySetup) {
    pBodySetup->ClearPhysicsMeshes();
  }
}
