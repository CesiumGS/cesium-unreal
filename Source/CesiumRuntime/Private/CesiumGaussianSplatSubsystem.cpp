#include "CesiumGaussianSplatSubsystem.h"

#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace {
FBox CalculateBounds(
    const TArray<UCesiumGltfGaussianSplatComponent*>& Components) {
  std::optional<FBox> Bounds;
  for (UCesiumGltfGaussianSplatComponent* Component : Components) {
    glm::mat4x4 Matrix = Component->GetMatrix();
    FBox ActorBounds = Component->GetBounds();
    glm::vec4 BoundsMin = glm::vec4(
                              ActorBounds.Min.X,
                              ActorBounds.Min.Y,
                              ActorBounds.Min.Z,
                              1.0) *
                          Matrix;
    glm::vec4 BoundsMax = glm::vec4(
                              ActorBounds.Max.X,
                              ActorBounds.Max.Y,
                              ActorBounds.Max.Z,
                              1.0) *
                          Matrix;
    if (Bounds) {
      Bounds->Min = FVector(
          std::min(Bounds->Min.X, (double)BoundsMin.x),
          std::min(Bounds->Min.Y, (double)BoundsMin.y),
          std::min(Bounds->Min.Z, (double)BoundsMin.z));
      Bounds->Max = FVector(
          std::max(Bounds->Max.X, (double)BoundsMax.x),
          std::max(Bounds->Max.Y, (double)BoundsMax.y),
          std::max(Bounds->Max.Z, (double)BoundsMax.z));
    } else {
      Bounds = FBox(
          FVector4(BoundsMin.x, BoundsMin.y, BoundsMin.z, BoundsMin.w),
          FVector4(BoundsMax.x, BoundsMax.y, BoundsMax.z, BoundsMax.w));
    }
  }
  return Bounds.value_or(FBox());
}
} // namespace

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

  this->NiagaraActor = SplatActor;
  this->NiagaraComponent =
      UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams);
  SplatActor->AddInstanceComponent(this->NiagaraComponent);
}

void UCesiumGaussianSplatSubsystem::RegisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  this->SplatComponents.Add(Component);
  this->SplatDelegateHandles.Emplace(Component->TransformUpdated.AddUObject(
      this,
      &UCesiumGaussianSplatSubsystem::OnTransformUpdated));

  this->NiagaraComponent->SetNiagaraVariableInt(
      TEXT("GridSize"),
      (int32)std::ceil(std::sqrt((double)this->GetNumSplats())));
  this->NiagaraComponent->SetSystemFixedBounds(
      CalculateBounds(this->SplatComponents));
  GetSplatInterface()->Refresh();
}

void UCesiumGaussianSplatSubsystem::UnregisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  int32 ComponentIndex = this->SplatComponents.IndexOfByKey(Component);
  check(ComponentIndex != INDEX_NONE);

  Component->TransformUpdated.Remove(
      this->SplatDelegateHandles[ComponentIndex]);
  this->SplatComponents.Remove(Component);
  this->SplatDelegateHandles.RemoveAt(ComponentIndex);
  GetSplatInterface()->Refresh();
}

void UCesiumGaussianSplatSubsystem::OnTransformUpdated(
    USceneComponent* UpdatedComponent,
    EUpdateTransformFlags UpdateTransformFlag,
    ETeleportType Teleport) {
  this->NiagaraComponent->SetSystemFixedBounds(
      CalculateBounds(this->SplatComponents));
  GetSplatInterface()->RefreshMatrices();
}

UCesiumGaussianSplatDataInterface*
UCesiumGaussianSplatSubsystem::GetSplatInterface() const {
  return CastChecked<UCesiumGaussianSplatDataInterface>(
      this->NiagaraComponent->GetDataInterface("SplatInterface"));
}
