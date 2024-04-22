// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumLifetime.h"
#include "CesiumRuntime.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Selection.h"
#endif
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "PhysicsEngine/BodySetup.h"
#include "Runtime/Launch/Resources/Version.h"
#include "StaticMeshResources.h"
#include "UObject/Object.h"
#include <algorithm>

/*static*/
AmortizedDestructor CesiumLifetime::amortizedDestructor = AmortizedDestructor();

/*static*/ void CesiumLifetime::destroy(UObject* pObject) {
  amortizedDestructor.destroy(pObject);
}

/*static*/ void
CesiumLifetime::destroyComponentRecursively(USceneComponent* pComponent) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::DestroyComponent)
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

#if WITH_EDITOR
  // If the editor is currently selecting this, remove the reference
  if (GEditor) {
    USelection* editorSelection = GEditor->GetSelectedComponents();
    if (editorSelection && editorSelection->IsSelected(pComponent))
      editorSelection->Deselect(pComponent);
  }
#endif

  pComponent->DestroyPhysicsState();
  pComponent->DestroyComponent();
  pComponent->ConditionalBeginDestroy();

  UE_LOG(LogCesium, VeryVerbose, TEXT("Destroying scene component done"));
}

void AmortizedDestructor::Tick(float DeltaTime) { processPending(); }

ETickableTickType AmortizedDestructor::GetTickableTickType() const {
  return ETickableTickType::Always;
}

bool AmortizedDestructor::IsTickableWhenPaused() const { return true; }

bool AmortizedDestructor::IsTickableInEditor() const { return true; }

TStatId AmortizedDestructor::GetStatId() const { return TStatId(); }

void AmortizedDestructor::destroy(UObject* pObject) {
  if (!runDestruction(pObject)) {
    addToPending(pObject);
  }
}

bool AmortizedDestructor::runDestruction(UObject* pObject) const {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::RunDestruction)

  if (!pObject) {
    return true;
  }

  pObject->MarkAsGarbage();

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

void AmortizedDestructor::addToPending(UObject* pObject) {
  _pending.Add(pObject);
}

void AmortizedDestructor::processPending() {
  std::swap(_nextPending, _pending);
  _pending.Empty();

  for (TWeakObjectPtr<UObject> pObject : _nextPending) {
    destroy(pObject.Get(true));
  }
}

void AmortizedDestructor::finalizeDestroy(UObject* pObject) const {
  // The freeing/clearing/destroying done here is normally done in these
  // objects' FinishDestroy method, but unfortunately we can't call that
  // directly without confusing the garbage collector if and when it _does_
  // run. So instead we manually release some critical resources here.

  UTexture2D* pTexture2D = Cast<UTexture2D>(pObject);
  if (pTexture2D) {
    FTexturePlatformData* pPlatformData = pTexture2D->GetPlatformData();
    pTexture2D->SetPlatformData(nullptr);
    delete pPlatformData;
  }

  UStaticMesh* pMesh = Cast<UStaticMesh>(pObject);
  if (pMesh) {
    pMesh->SetRenderData(nullptr);
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
