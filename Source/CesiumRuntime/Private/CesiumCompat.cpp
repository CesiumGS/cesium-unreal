#include "CesiumCompat.h"
#include "CesiumCommon.h"

FSceneInterfaceWrapper::FSceneInterfaceWrapper(
    FSceneInterface* InSceneInterface) {
#if ENGINE_VERSION_5_7_OR_HIGHER
  this->ShaderPlatform = InSceneInterface->GetShaderPlatform();
#endif
  this->RHIFeatureLevelType = InSceneInterface->GetFeatureLevel();
}

FMaterialRelevance FSceneInterfaceWrapper::GetMaterialRelevance(
    UMeshComponent* InMeshComponent) const {
#if ENGINE_VERSION_5_7_OR_HIGHER
  return InMeshComponent->GetMaterialRelevance(this->ShaderPlatform);
#else
  return InMeshComponent->GetMaterialRelevance(this->RHIFeatureLevelType);
#endif
}

void ALevelInstance_SetDesiredRuntimeBehavior(
    ALevelInstance* Instance,
    ELevelInstanceRuntimeBehavior RuntimeBehavior) {
#if WITH_EDITORONLY_DATA && ENGINE_VERSION_5_7_OR_HIGHER
  Instance->SetDesiredRuntimeBehavior(RuntimeBehavior);
#elif WITH_EDITORONLY_DATA
  Instance->DesiredRuntimeBehavior = RuntimeBehavior;
#endif
}
