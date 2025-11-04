#include "CesiumCompat.h"

FSceneInterfaceWrapper::FSceneInterfaceWrapper(
    FSceneInterface* InSceneInterface) {
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 7
  this->ShaderPlatform = InSceneInterface->GetShaderPlatform();
#endif
  this->RHIFeatureLevelType = InSceneInterface->GetFeatureLevel();
}

FMaterialRelevance FSceneInterfaceWrapper::GetMaterialRelevance(
    UMeshComponent* InMeshComponent) const {
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 7
  return InMeshComponent->GetMaterialRelevance(this->ShaderPlatform);
#else
  return InMeshComponent->GetMaterialRelevance(this->RHIFeatureLevelType);
#endif
}

void ALevelInstance_SetDesiredRuntimeBehavior(
    ALevelInstance* Instance,
    ELevelInstanceRuntimeBehavior RuntimeBehavior) {
#if WITH_EDITORONLY_DATA && ENGINE_MAJOR_VERSION >= 5 &&                       \
    ENGINE_MINOR_VERSION >= 7
  return Instance->SetDesiredRuntimeBehavior(RuntimeBehavior);
#elif WITH_EDITORONLY_DATA
  return Instance->DesiredRuntimeBehavior = RuntimeBehavior;
#endif
}
