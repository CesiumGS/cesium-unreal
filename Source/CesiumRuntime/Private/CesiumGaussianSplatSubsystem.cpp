#include "CesiumGaussianSplatSubsystem.h"

#include "NiagaraActor.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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

UCesiumGaussianSplatSubsystem::UCesiumGaussianSplatSubsystem() {
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UNiagaraSystem> NiagaraSystem;
    FConstructorStatics()
        : NiagaraSystem(TEXT(
              "/Script/Niagara.NiagaraSystem'/CesiumForUnreal/GaussianSplatSystem.GaussianSplatSystem'")) {
    }
  };

  static FConstructorStatics Statics;
  this->NiagaraSystemAsset = Statics.NiagaraSystem.Object;
}

void UCesiumGaussianSplatSubsystem::OnWorldBeginPlay(UWorld& InWorld) {
  FFXSystemSpawnParameters SpawnParams;
  SpawnParams.WorldContextObject = &InWorld;
  SpawnParams.SystemTemplate = this->NiagaraSystemAsset;
  SpawnParams.bAutoDestroy = false;


  this->NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocationWithParams(SpawnParams);
}

void UCesiumGaussianSplatSubsystem::RegisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  this->SplatComponents.Add(Component);
  this->SplatDelegateHandles.Emplace(Component->TransformUpdated.AddUObject(
      this,
      &UCesiumGaussianSplatSubsystem::OnTransformUpdated));

  this->NiagaraComponent->SetSystemFixedBounds(
      CalculateBounds(this->SplatComponents));
}

void UCesiumGaussianSplatSubsystem::UnregisterSplat(
    UCesiumGltfGaussianSplatComponent* Component) {
  int32 ComponentIndex = this->SplatComponents.IndexOfByKey(Component);
  check(ComponentIndex != INDEX_NONE);

  Component->TransformUpdated.Remove(
      this->SplatDelegateHandles[ComponentIndex]);
  this->SplatComponents.Remove(Component);
  this->SplatDelegateHandles.RemoveAt(ComponentIndex);
}

void UCesiumGaussianSplatSubsystem::OnTransformUpdated(
    USceneComponent* UpdatedComponent,
    EUpdateTransformFlags UpdateTransformFlag,
    ETeleportType Teleport) {
  this->NiagaraComponent->SetSystemFixedBounds(
      CalculateBounds(this->SplatComponents));
}

UCesiumGaussianSplatDataInterface*
UCesiumGaussianSplatSubsystem::GetSplatInterface() const {
  return CastChecked<UCesiumGaussianSplatDataInterface>(
      this->NiagaraComponent->GetDataInterface("SplatInterface"));
}
