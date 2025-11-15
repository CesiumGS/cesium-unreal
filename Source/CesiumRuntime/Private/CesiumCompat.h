// Copyright 2020 - 2025 CesiumGS, Inc.and Contributors

#pragma once

#include "CesiumCommon.h"
#include "Components/MeshComponent.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "RHIFeatureLevel.h"
#include "RHIShaderPlatform.h"
#include "SceneInterface.h"

/**
 * Wrapper around FSceneInterface to deal with the switch to EShaderPlatform in
 * Unreal 5.7.
 */
struct FSceneInterfaceWrapper {
  FSceneInterfaceWrapper(FSceneInterface* SceneInterface);

  FMaterialRelevance
  GetMaterialRelevance(UMeshComponent* InMeshComponent) const;

  ERHIFeatureLevel::Type RHIFeatureLevelType;

#if ENGINE_VERSION_5_7_OR_HIGHER
private:
  EShaderPlatform ShaderPlatform;
#endif
};

/**
 * Compatibility fix for the DesiredRuntimeBehavior -> SetDesiredRuntimeBehavior
 * change in Unreal 5.7.
 */
void ALevelInstance_SetDesiredRuntimeBehavior(
    ALevelInstance* Instance,
    ELevelInstanceRuntimeBehavior RuntimeBehavior);
