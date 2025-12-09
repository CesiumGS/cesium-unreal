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

const int32 RESET_FRAME_COUNT = 5;

namespace {
FBox CalculateBounds(
    const TArray<UCesiumGltfGaussianSplatComponent*>& Components) {
  std::optional<FBox> Bounds;
  for (UCesiumGltfGaussianSplatComponent* Component : Components) {
    check(Component);
    const FTransform& ComponentTransform = Component->GetComponentTransform();
    FBox ActorBounds = Component->GetBounds();
    FVector Center = ComponentTransform.GetTranslation();
    FBox TransformedBounds{
        ComponentTransform.TransformPositionNoScale(ActorBounds.Min),
        ComponentTransform.TransformPositionNoScale(ActorBounds.Max)};

    FVector BoundsMin = TransformedBounds.Min;
    FVector BoundsMax = TransformedBounds.Max;
    if (Bounds) {
      Bounds->Min = FVector(
          std::min(Bounds->Min.X, (double)BoundsMin.X),
          std::min(Bounds->Min.Y, (double)BoundsMin.Y),
          std::min(Bounds->Min.Z, (double)BoundsMin.Z));
      Bounds->Max = FVector(
          std::max(Bounds->Max.X, (double)BoundsMax.X),
          std::max(Bounds->Max.Y, (double)BoundsMax.Y),
          std::max(Bounds->Max.Z, (double)BoundsMax.Z));
    } else {
      Bounds = FBox(
          FVector4(
              BoundsMin.X - 1.0,
              BoundsMin.Y - 1.0,
              BoundsMin.Z - 1.0,
              0.0),
          FVector4(
              BoundsMax.X + 1.0,
              BoundsMax.Y + 1.0,
              BoundsMax.Z + 1.0,
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

int32 UCesiumGaussianSplatSubsystem::GetNumSplats() const {
  return this->NumSplats;
}

void UCesiumGaussianSplatSubsystem::InitializeForWorld(UWorld& InWorld) {
  for (ACesiumGaussianSplatActor* Actor :
       TActorRange<ACesiumGaussianSplatActor>(&InWorld)) {
    // Actor singleton already exists in the world (usually means we stopped a
    // PIE session and returned to the editor world).
    this->LastCreatedWorld = &InWorld;
    this->NiagaraActor = Actor;
    this->NiagaraComponent =
        this->NiagaraActor->FindComponentByClass<UNiagaraComponent>();
    this->RecomputeBounds();
    this->UpdateNiagaraComponent();
    return;
  }

  FActorSpawnParameters ActorParams;
  ActorParams.Name = FName(TEXT("GaussianSplatSystemActor"));
  ActorParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
#if WITH_EDITOR
  ActorParams.bTemporaryEditorActor = true;
  ActorParams.bHideFromSceneOutliner = true;
#endif
  ACesiumGaussianSplatActor* SplatActor =
      InWorld.SpawnActor<ACesiumGaussianSplatActor>(ActorParams);
  SplatActor->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  USceneComponent* SceneComponent =
      CastChecked<USceneComponent>(SplatActor->AddComponentByClass(
          USceneComponent::StaticClass(),
          false,
          FTransform(),
          false));
  SplatActor->AddInstanceComponent(SceneComponent);

  UObject* MaybeSplatNiagaraSystem = StaticLoadObject(
      UNiagaraSystem::StaticClass(),
      nullptr,
      TEXT(
          "/Script/Niagara.NiagaraSystem'/CesiumForUnreal/GaussianSplatting/GaussianSplatSystem.GaussianSplatSystem'"));
  if (!IsValid(MaybeSplatNiagaraSystem)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Unable to initialize gaussian splat Niagara system."));
    return;
  }

  FFXSystemSpawnParameters SpawnParams;
  SpawnParams.WorldContextObject = &InWorld;
  SpawnParams.SystemTemplate =
      CastChecked<UNiagaraSystem>(MaybeSplatNiagaraSystem);
  SpawnParams.bAutoDestroy = false;
  SpawnParams.AttachToComponent = SceneComponent;
  SpawnParams.bAutoActivate = true;

  this->LastCreatedWorld = &InWorld;
  this->NiagaraComponent =
      UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams);
  if (!IsValid(this->NiagaraComponent)) {
    // Failed to create niagara component - make sure it's null and we'll try
    // again next time.
    this->NiagaraComponent = nullptr;
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Unable to create gaussian splat Niagara component"));
    return;
  }
  this->NiagaraComponent->Activate();
  this->NiagaraActor = SplatActor;
  SplatActor->AddInstanceComponent(this->NiagaraComponent);

  this->UpdateNiagaraComponent();
}

void UCesiumGaussianSplatSubsystem::Initialize(
    FSubsystemCollectionBase& Collection) {
  // Unreal will call `Tick` on the Class Default Object for this subsystem. We
  // don't want that to happen, because this is supposed to be a singleton, and
  // doing so will result in multiple actors being spawned.
  //
  // Because `Initialize` is never called on the CDO, we can use this as a
  // marker of whether we're in the *true* singleton instance of this subsystem.
  bIsTickEnabled = true;
}

void UCesiumGaussianSplatSubsystem::Deinitialize() {
  bIsTickEnabled = false;
  if (IsValid(this->NiagaraActor)) {
    this->NiagaraActor->Destroy();
  }

  this->NiagaraActor = nullptr;
  this->NiagaraComponent = nullptr;
  this->LastCreatedWorld = nullptr;
}

void UCesiumGaussianSplatSubsystem::RegisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  check(Component);

  if (IsValid(this->NiagaraComponent)) {
    // Lock buffers when adding components to avoid adding components while
    // uploading previous components to GPU
    FScopeLock ScopeLock = this->GetSplatInterface()->LockGaussianBuffers();
    this->SplatComponents.Add(Component);
    this->NumSplats += Component->NumSplats;
  } else {
    this->SplatComponents.Add(Component);
    this->NumSplats += Component->NumSplats;
  }

  this->UpdateNiagaraComponent();
}

void UCesiumGaussianSplatSubsystem::UnregisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  check(Component);

  if (IsValid(this->NiagaraComponent)) {
    FScopeLock ScopeLock = this->GetSplatInterface()->LockGaussianBuffers();
    this->SplatComponents.Remove(Component);
    this->NumSplats -= Component->NumSplats;
  } else {
    this->SplatComponents.Remove(Component);
    this->NumSplats -= Component->NumSplats;
  }

  this->UpdateNiagaraComponent();
}

void UCesiumGaussianSplatSubsystem::RecomputeBounds() {
  if (IsValid(this->NiagaraComponent)) {
    FBox Bounds = CalculateBounds(this->SplatComponents);
    const FTransform& NiagaraToWorld =
        this->NiagaraComponent->GetComponentTransform();
    this->NiagaraComponent->SetSystemFixedBounds(Bounds);
    UE_LOG(LogCesium, Log, TEXT("Setting bounds: %s"), *Bounds.ToString());
    GetSplatInterface()->RefreshMatrices();
  }
}

void UCesiumGaussianSplatSubsystem::UpdateNiagaraComponent() {
  if (IsValid(this->NiagaraComponent)) {
    this->NiagaraComponent->SetVariableInt(
        FName(TEXT("GridSize")),
        (int32)std::ceil(std::cbrt((double)this->NumSplats)));
    GetSplatInterface()->Refresh();
    this->bNeedsReset = true;
    this->ResetFrameCounter = RESET_FRAME_COUNT;
  }
}

UCesiumGaussianSplatDataInterface*
UCesiumGaussianSplatSubsystem::GetSplatInterface() const {
  return UNiagaraFunctionLibrary::GetDataInterface<
      UCesiumGaussianSplatDataInterface>(
      this->NiagaraComponent,
      FName(TEXT("SplatInterface")));
}

void UCesiumGaussianSplatSubsystem::Tick(float DeltaTime) {
  if (!this->bIsTickEnabled) {
    return;
  }

  if (this->bNeedsReset) {
    this->ResetFrameCounter -= 1;
  }

  UWorld* World = GetPrimaryWorld();

  if (!IsValid(World)) {
    if (IsValid(this->NiagaraActor)) {
      this->NiagaraActor->Destroy();
    }

    this->NiagaraActor = nullptr;
    this->NiagaraComponent = nullptr;
    this->LastCreatedWorld = nullptr;
    return;
  }

  // Gaussian splats are purely visual, so we don't need to initialize them if
  // we can't render anything. Besides, even if we'd *want* to, we will fail to
  // spawn the Niagara component if either of these conditions are false (they
  // are checked in UNiagaraFunctionLibrary::CreateNiagaraSystem).
  if (!FApp::CanEverRender() || World->IsNetMode(NM_DedicatedServer)) {
    return;
  }

  if (IsValid(this->NiagaraActor)) {
    if (this->bNeedsReset && this->ResetFrameCounter <= 0) {
      // We want to avoid calling ResetSystem multiple times a frame, so we
      // combine the calls into one.
      this->bNeedsReset = false;
      this->NiagaraComponent->ResetSystem();
    }
  }

  if (IsValid(this->NiagaraActor) && World == this->LastCreatedWorld) {
    return;
  }

  this->InitializeForWorld(*World);
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
  return this->bIsTickEnabled;
}
