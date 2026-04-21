// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#include "CesiumGaussianSplatSubsystem.h"

#include "CesiumRuntime.h"

#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "RHIGlobals.h"

#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"

namespace {
FBox CalculateBounds(
    const TArray<UCesiumGltfGaussianSplatComponent*>& Components) {
  std::optional<FBox> Bounds;
  for (const UCesiumGltfGaussianSplatComponent* Component : Components) {
    check(Component);
    const FTransform& ComponentTransform = Component->GetComponentTransform();
    const FBox& ActorBounds = Component->Data.Bounds;
    FBox TransformedBounds{
        ComponentTransform.TransformPosition(ActorBounds.Min),
        ComponentTransform.TransformPosition(ActorBounds.Max)};

    FVector BoundsMin = TransformedBounds.Min;
    FVector BoundsMax = TransformedBounds.Max;
    if (Bounds) {
      Bounds->Min = FVector(
          std::min(Bounds->Min.X, std::min(BoundsMin.X, BoundsMax.X)),
          std::min(Bounds->Min.Y, std::min(BoundsMin.Y, BoundsMax.Y)),
          std::min(Bounds->Min.Z, std::min(BoundsMin.Z, BoundsMax.Z)));
      Bounds->Max = FVector(
          std::max(Bounds->Max.X, std::max(BoundsMin.X, BoundsMax.X)),
          std::max(Bounds->Max.Y, std::max(BoundsMin.Y, BoundsMax.Y)),
          std::max(Bounds->Max.Z, std::max(BoundsMin.Z, BoundsMax.Z)));
    } else {
      Bounds = FBox(
          FVector4(
              BoundsMin.X - 1.0,
              std::min(BoundsMin.Y, BoundsMax.Y) - 1.0,
              std::min(BoundsMin.Z, BoundsMax.Z) - 1.0,
              0.0),
          FVector4(
              std::max(BoundsMin.X, BoundsMax.X) - 1.0,
              std::max(BoundsMin.Y, BoundsMax.Y) - 1.0,
              std::max(BoundsMin.Z, BoundsMax.Z) - 1.0,
              0.0));
    }
  }
  return Bounds.value_or(
      FBox(FVector4(-1.0, -1.0, -1.0, 0.0), FVector4(1.0, 1.0, 1.0, 0.0)));
}

// Running in a build, there is only one world context at a time. However, in
// play-in-editor, there can be both the editor world context and the
// play-in-editor world context.
//
// We need to choose the "primary world." Technically, it would be best to
// support *all* available world contexts, but considering that we are talking
// about uploading potentially multiple gigabytes of data to the GPU per
// instance, it seems like it might not be wise to be doing it more than once at
// a time unless necessary.
UWorld* GetPrimaryWorld() {
#if WITH_EDITOR
  if (!IsValid(GEditor)) {
    return nullptr;
  }

  const TIndirectArray<FWorldContext>& Contexts = GEditor->GetWorldContexts();
#else
  if (!IsValid(GEngine)) {
    return nullptr;
  }

  const TIndirectArray<FWorldContext>& Contexts = GEngine->GetWorldContexts();
#endif

  if (Contexts.IsEmpty()) {
    return nullptr;
  }

  for (const FWorldContext& Context : Contexts) {
    if (Context.bIsPrimaryPIEInstance) {
      return Context.World();
    }
  }

  return Contexts[0].World();
}
} // namespace

/*static*/ UCesiumGaussianSplatSubsystem* UCesiumGaussianSplatSubsystem::Get() {
  return IsValid(GEngine)
             ? GEngine->GetEngineSubsystem<UCesiumGaussianSplatSubsystem>()
             : nullptr;
}

void UCesiumGaussianSplatSubsystem::Initialize(
    FSubsystemCollectionBase& Collection) {
  // Unreal will call `Tick` on the Class Default Object for this subsystem.
  // Since this is supposed to be a singleton, we prevent this from happening;
  // otherwise, multiple actors will be spawned.
  //
  // Because `Initialize` is never called on the CDO, we can use this as a
  // marker of whether we're in the *true* singleton instance of this subsystem.
  this->_isTickEnabled = true;
}

void UCesiumGaussianSplatSubsystem::Deinitialize() {
  this->_isTickEnabled = false;

  if (IsValid(this->_pNiagaraActor)) {
    this->_pNiagaraActor->Destroy();
  }

  this->reset();
}

void UCesiumGaussianSplatSubsystem::RegisterSplat(
    UCesiumGltfGaussianSplatComponent* pComponent) {
  check(pComponent);
  this->SplatComponents.Add(pComponent);
  this->_splatCount += pComponent->Data.NumSplats;
  this->makeInterfaceDirty();

  this->_newComponents.Add(pComponent);
}

void UCesiumGaussianSplatSubsystem::UnregisterSplat(
    UCesiumGltfGaussianSplatComponent* pComponent) {
  check(pComponent);
  this->SplatComponents.Remove(pComponent);
  this->_splatCount -= pComponent->Data.NumSplats;
  this->makeInterfaceDirty();

  this->_newComponents.Remove(pComponent);
}

void UCesiumGaussianSplatSubsystem::RecomputeBounds() {
  if (!IsValid(this->_pNiagaraComponent)) {
    return;
  }

  const FBox Bounds = CalculateBounds(this->SplatComponents);
  this->_pNiagaraComponent->SetSystemFixedBounds(Bounds);
  this->makeInterfaceDirty(true /* tilesOnly */);
}

void UCesiumGaussianSplatSubsystem::initializeForWorld(UWorld& InWorld) {
  for (ACesiumGaussianSplatActor* pActor :
       TActorRange<ACesiumGaussianSplatActor>(&InWorld)) {
    // Actor singleton already exists in the world (usually means we stopped a
    // PIE session and returned to the editor world).
    this->_pLastCreatedWorld = &InWorld;
    this->_pNiagaraActor = pActor;
    this->_pNiagaraComponent =
        this->_pNiagaraActor->FindComponentByClass<UNiagaraComponent>();
    this->RecomputeBounds();
    return;
  }

  FActorSpawnParameters ActorParams;
  ActorParams.Name = FName(TEXT("CesiumGaussianSplatSystemActor"));
  ActorParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
#if WITH_EDITOR
  ActorParams.bTemporaryEditorActor = true;
  // Set this to false if you need the actor to be visible in the outliner to
  // help with debugging.
  ActorParams.bHideFromSceneOutliner = true;
#endif

  ACesiumGaussianSplatActor* pActor =
      InWorld.SpawnActor<ACesiumGaussianSplatActor>(ActorParams);
  check(pActor);
  pActor->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  USceneComponent* pSceneComponent =
      CastChecked<USceneComponent>(pActor->AddComponentByClass(
          USceneComponent::StaticClass(),
          false,
          FTransform(),
          false));
  pActor->AddInstanceComponent(pSceneComponent);

  UObject* pNiagaraSystem = StaticLoadObject(
      UNiagaraSystem::StaticClass(),
      nullptr,
      NiagaraSystemAssetPath);
  if (!IsValid(pNiagaraSystem)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Unable to initialize Niagara system for Gaussian splats."));
    return;
  }

  FFXSystemSpawnParameters SpawnParams;
  SpawnParams.WorldContextObject = &InWorld;
  SpawnParams.SystemTemplate = CastChecked<UNiagaraSystem>(pNiagaraSystem);
  SpawnParams.bAutoDestroy = false;
  SpawnParams.AttachToComponent = pSceneComponent;
  SpawnParams.bAutoActivate = true;

  this->_pLastCreatedWorld = &InWorld;
  this->_pNiagaraComponent =
      UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams);
  if (!IsValid(this->_pNiagaraComponent)) {
    // Failed to create niagara component - make sure it's null and we'll try
    // again next time.
    this->_pNiagaraComponent = nullptr;
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Unable to create Niagara component to render Gaussian Splats."));
    return;
  }

  this->_pNiagaraComponent->Activate();
  this->_pNiagaraActor = pActor;

  pActor->AddInstanceComponent(this->_pNiagaraComponent);
  this->makeInterfaceDirty();
}

void UCesiumGaussianSplatSubsystem::Tick(float DeltaTime) {
  if (!this->_isTickEnabled) {
    return;
  }

  UWorld* pWorld = GetPrimaryWorld();
  // Gaussian splats are purely visual, so we don't need to initialize them if
  // we can't render anything. Besides, the Niagara component will fail to
  // spawn if either of these conditions are false (they are checked in
  // UNiagaraFunctionLibrary::CreateNiagaraSystem).
  if (!FApp::CanEverRender() || pWorld->IsNetMode(NM_DedicatedServer)) {
    return;
  }

  if (!IsValid(pWorld)) {
    if (IsValid(this->_pNiagaraActor)) {
      this->_pNiagaraActor->Destroy();
    }
    this->reset();
    return;
  }

  if (!IsValid(this->_pNiagaraActor) || pWorld != this->_pLastCreatedWorld) {
    this->initializeForWorld(*pWorld);
  }

  UCesiumGaussianSplatDataInterface* pDataInterface = this->getDataInterface();
  if (!pDataInterface || !this->_splatInterfaceDirty) {
    return;
  }

  // If no splats changed this frame, but the interface is still dirty, then
  // we're waiting on the interface to complete its update.
  if (pDataInterface->IsUpdatingForWorld(pWorld)) {
    // Pausing the Niagara system prevents incorrect LOD pop-ins as the buffers
    // update.
    this->_pNiagaraComponent->SetPaused(true);
  } else {
    this->_splatInterfaceDirty = false;

    if (this->_newComponents.Num()) {
      TSet<const UCesiumGltfGaussianSplatComponent*> componentsInUpdate =
          pDataInterface->GetComponentsInUpdateForWorld(pWorld);

      for (UCesiumGltfGaussianSplatComponent* pComponent :
           this->_newComponents) {
        if (componentsInUpdate.Contains(pComponent)) {
          componentsInUpdate.Remove(pComponent);

          check(pComponent->registerWithSubsystemPromise);
          pComponent->registerWithSubsystemPromise->resolve(true);
        }
      }
    }

    this->_pNiagaraComponent->SetVariableInt(
        FName(TEXT("SplatCount")),
        this->_splatCount);
    this->_pNiagaraComponent->SetPaused(false);
  }
}

ETickableTickType UCesiumGaussianSplatSubsystem::GetTickableTickType() const {
  return ETickableTickType::Always;
}

TStatId UCesiumGaussianSplatSubsystem::GetStatId() const {
  RETURN_QUICK_DECLARE_CYCLE_STAT(
      UCesiumGaussianSplatSubsystem,
      STATGROUP_Tickables);
}

bool UCesiumGaussianSplatSubsystem::IsTickableWhenPaused() const {
  return true;
}

bool UCesiumGaussianSplatSubsystem::IsTickableInEditor() const { return true; }

bool UCesiumGaussianSplatSubsystem::IsTickable() const {
  return this->_isTickEnabled;
}

void UCesiumGaussianSplatSubsystem::reset() {
  this->_pNiagaraActor = nullptr;
  this->_pNiagaraComponent = nullptr;
  this->_pLastCreatedWorld = nullptr;
  this->_newComponents.Empty();

  this->SplatComponents.Empty();
  this->_splatCount = 0;
  this->makeInterfaceDirty();
}

void UCesiumGaussianSplatSubsystem::makeInterfaceDirty(bool tilesOnly) {
  UCesiumGaussianSplatDataInterface* pDataInterface = this->getDataInterface();
  if (!pDataInterface) {
    return;
  }

  if (tilesOnly) {
    pDataInterface->MarkTilesDirty();
  } else {
    pDataInterface->MarkDirty();
  }

  this->_splatInterfaceDirty = true;
}

UCesiumGaussianSplatDataInterface*
UCesiumGaussianSplatSubsystem::getDataInterface() const {
  return UNiagaraFunctionLibrary::GetDataInterface<
      UCesiumGaussianSplatDataInterface>(
      this->_pNiagaraComponent,
      FName(TEXT("SplatInterface")));
}
