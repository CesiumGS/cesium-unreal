#include "CesiumGaussianSplatSubsystem.h"

#include "CesiumRuntime.h"

#include "EngineUtils.h"

#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace {
FBox CalculateBounds(
    const TArray<UCesiumGltfGaussianSplatComponent*>& Components) {
  std::optional<FBox> Bounds;
  for (UCesiumGltfGaussianSplatComponent* Component : Components) {
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
          FVector4(BoundsMin.X, BoundsMin.Y, BoundsMin.Z, 0.0),
          FVector4(BoundsMax.X, BoundsMax.Z, BoundsMax.Z, 0.0));
    }
  }
  return Bounds.value_or(FBox());
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
  int32 Num = 0;
  for (UCesiumGltfGaussianSplatComponent* Component : this->SplatComponents) {
    Num += Component->NumSplats;
  }

  return Num;
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
#endif
  ACesiumGaussianSplatActor* SplatActor =
      InWorld.SpawnActor<ACesiumGaussianSplatActor>(ActorParams);

  USceneComponent* SceneComponent =
      CastChecked<USceneComponent>(SplatActor->AddComponentByClass(
          USceneComponent::StaticClass(),
          false,
          FTransform(),
          false));
  SplatActor->AddInstanceComponent(SceneComponent);

  UNiagaraSystem* SplatNiagaraSystem = CastChecked<
      UNiagaraSystem>(StaticLoadObject(
      UNiagaraSystem::StaticClass(),
      nullptr,
      TEXT(
          "/Script/Niagara.NiagaraSystem'/CesiumForUnreal/GaussianSplatSystem.GaussianSplatSystem'")));

  FFXSystemSpawnParameters SpawnParams;
  SpawnParams.WorldContextObject = &InWorld;
  SpawnParams.SystemTemplate = SplatNiagaraSystem;
  SpawnParams.bAutoDestroy = false;
  SpawnParams.AttachToComponent = SceneComponent;
  SpawnParams.bAutoActivate = true;

  this->LastCreatedWorld = &InWorld;
  this->NiagaraActor = SplatActor;
  this->NiagaraComponent =
      UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams);
  this->NiagaraComponent->SetAutoActivate(true);
  this->NiagaraComponent->Activate();
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
  if (IsValid(this->NiagaraComponent)) {
    // Lock buffers when adding components to avoid adding components while
    // uploading previous components to GPU
    FScopeLock ScopeLock = this->GetSplatInterface()->LockGaussianBuffers();
    this->SplatComponents.Add(Component);
  } else {
    this->SplatComponents.Add(Component);
  }

  this->UpdateNiagaraComponent();
}

void UCesiumGaussianSplatSubsystem::UnregisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  if (IsValid(this->NiagaraComponent)) {
    FScopeLock ScopeLock = this->GetSplatInterface()->LockGaussianBuffers();
    this->SplatComponents.Remove(Component);
  } else {
    this->SplatComponents.Remove(Component);
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
        (int32)std::ceil(std::sqrt((double)this->GetNumSplats())));
    GetSplatInterface()->Refresh();
    this->NiagaraComponent->ResetSystem();
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

  if (World == this->LastCreatedWorld) {
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
