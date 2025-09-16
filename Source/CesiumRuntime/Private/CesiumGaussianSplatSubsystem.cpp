#include "CesiumGaussianSplatSubsystem.h"

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
        ComponentTransform.TransformPositionNoScale(
            ActorBounds.Min),
        ComponentTransform.TransformPositionNoScale(
            ActorBounds.Max)};
    
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
} // namespace

UCesiumGaussianSplatSubsystem::UCesiumGaussianSplatSubsystem()
    : UWorldSubsystem() {}

int32 UCesiumGaussianSplatSubsystem::GetNumSplats() const {
  int32 Num = 0;
  for (UCesiumGltfGaussianSplatComponent* Component : this->SplatComponents) {
    Num += Component->NumSplats;
  }

  return Num;
}

void UCesiumGaussianSplatSubsystem::OnWorldBeginPlay(UWorld& InWorld) {
  FActorSpawnParameters ActorParams;
  ActorParams.Name = FName(TEXT("GaussianSplatSystemActor"));
  AActor* SplatActor = InWorld.SpawnActor<AActor>(ActorParams);

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

  this->NiagaraActor = SplatActor;
  this->NiagaraComponent =
      UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams);
  this->NiagaraComponent->SetAutoActivate(true);
  this->NiagaraComponent->Activate();
  SplatActor->AddInstanceComponent(this->NiagaraComponent);

  this->GetSplatInterface()->SetGaussianSplatSubsystem(this);

  this->UpdateNiagaraComponent();
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
  this->SplatDelegateHandles.Emplace(Component->TransformUpdated.AddUObject(
      this,
      &UCesiumGaussianSplatSubsystem::OnTransformUpdated));

  this->UpdateNiagaraComponent();
}

void UCesiumGaussianSplatSubsystem::UnregisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  int32 ComponentIndex = this->SplatComponents.IndexOfByKey(Component);
  check(ComponentIndex != INDEX_NONE);

  Component->TransformUpdated.Remove(
      this->SplatDelegateHandles[ComponentIndex]);

  if (IsValid(this->NiagaraComponent)) {
    FScopeLock ScopeLock = this->GetSplatInterface()->LockGaussianBuffers();
    this->SplatComponents.Remove(Component);
  } else {
    this->SplatComponents.Remove(Component);
  }
  this->SplatDelegateHandles.RemoveAt(ComponentIndex);

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

void UCesiumGaussianSplatSubsystem::OnTransformUpdated(
    USceneComponent* UpdatedComponent,
    EUpdateTransformFlags UpdateTransformFlag,
    ETeleportType Teleport) {
  this->RecomputeBounds();
}

void UCesiumGaussianSplatSubsystem::UpdateNiagaraComponent() {
  if (IsValid(this->NiagaraComponent)) {
    this->NiagaraComponent->SetNiagaraVariableInt(
        TEXT("GridSize"),
        (int32)std::ceil(std::sqrt((double)this->GetNumSplats())));
    GetSplatInterface()->Refresh();
    this->NiagaraComponent->ResetSystem();
  }
}

UCesiumGaussianSplatDataInterface*
UCesiumGaussianSplatSubsystem::GetSplatInterface() const {
  return CastChecked<UCesiumGaussianSplatDataInterface>(
      this->NiagaraComponent->GetDataInterface("SplatInterface"));
}
