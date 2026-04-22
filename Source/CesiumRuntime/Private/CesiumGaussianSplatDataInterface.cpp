// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#include "CesiumGaussianSplatDataInterface.h"

#include "CesiumGaussianSplatSubsystem.h"
#include "CesiumGltfGaussianSplatComponent.h"
#include "CesiumRuntime.h"

#include "Containers/Map.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "HLSLTypeAliases.h"
#include "Misc/FileHelper.h"
#include "NiagaraCommon.h"
#include "NiagaraCompileHashVisitor.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterface.h"
#include "NiagaraRenderer.h"
#include "NiagaraShaderParametersBuilder.h"
#include "NiagaraSystemInstance.h"
#include "RHICommandList.h"
#include "ShaderCore.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

const FString ComputeSplatFunctionName = TEXT("ComputeSplat");

FNiagaraDataInterfaceProxyCesiumGaussianSplats::
    ~FNiagaraDataInterfaceProxyCesiumGaussianSplats() {
  this->TileIndicesBuffer.Release();
  this->TileTransformsBuffer.Release();
  this->SplatSHDegreesBuffer.Release();
  this->PositionsBuffer.Release();
  this->ScalesBuffer.Release();
  this->OrientationsBuffer.Release();
  this->ColorsBuffer.Release();
  this->SHNonZeroCoeffsBuffer.Release();
}

void FNDICesiumGaussianSplats_InstanceData::reset() {
  this->Components.Empty();
  this->SplatCount = 0;
  this->SplatsFence.reset();
  this->MatricesFence.reset();
}

namespace {
void updateTileTransforms(
    FRHICommandListImmediate& RHICmdList,
    const TArray<const UCesiumGltfGaussianSplatComponent*>& Components,
    FReadBuffer& Buffer) {
  if (Components.IsEmpty()) {
    Buffer.Release();
    return;
  }

  // Each tile corresponds to ten vectors:
  // 0: Tile transform row 0
  // 1: Tile transform row 1
  // 2: Tile transform row 2
  // 3: Tile transform row 3
  // 4: Inverse tile transform row 0
  // 5: Inverse tile transform row 1
  // 6: Inverse tile transform row 2
  // 7: Inverse tile transform row 3
  // 8: Tile scale (xyz), visibility (w)
  // 9: Tile rotation
  const int32 vectorsPerComponent = 10;

  const int32 totalVectorCount = Components.Num() * vectorsPerComponent;
  const uint32 requiredByteLength = totalVectorCount * sizeof(FVector4f);

  if (Buffer.NumBytes != requiredByteLength) {
    Buffer.Release();
    Buffer.Initialize(
        RHICmdList,
        TEXT(
            "FNiagaraDataInterfaceProxyCesiumGaussianSplat_TileTransformsBuffer"),
        sizeof(FVector4f),
        totalVectorCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);
  }

  FVector4f* pBufferData = static_cast<FVector4f*>(RHICmdList.LockBuffer(
      Buffer.Buffer,
      0,
      requiredByteLength,
      EResourceLockMode::RLM_WriteOnly));

  for (int32 i = 0; i < Components.Num(); i++) {
    const UCesiumGltfGaussianSplatComponent* pComponent = Components[i];
    check(pComponent);

    const int32 Offset = i * vectorsPerComponent;

    glm::mat4 tileMatrix(pComponent->GetMatrix());
    const FTransform& componentToWorld = pComponent->GetComponentToWorld();
    FVector tileScale = componentToWorld.GetScale3D();
    FQuat tileRotation = componentToWorld.GetRotation();
    tileRotation.Normalize();

    // Write in row-order for easy access in HLSL.
    pBufferData[Offset + 0] = FVector4f(
        tileMatrix[0][0],
        tileMatrix[1][0],
        tileMatrix[2][0],
        tileMatrix[3][0]);
    pBufferData[Offset + 1] = FVector4f(
        tileMatrix[0][1],
        tileMatrix[1][1],
        tileMatrix[2][1],
        tileMatrix[3][1]);
    pBufferData[Offset + 2] = FVector4f(
        tileMatrix[0][2],
        tileMatrix[1][2],
        tileMatrix[2][2],
        tileMatrix[3][2]);
    pBufferData[Offset + 3] = FVector4f(
        tileMatrix[0][3],
        tileMatrix[1][3],
        tileMatrix[2][3],
        tileMatrix[3][3]);

    glm::dmat4 inverseTileMatrix(glm::inverse(pComponent->GetMatrix()));
    pBufferData[Offset + 4] = FVector4f(
        inverseTileMatrix[0][0],
        inverseTileMatrix[1][0],
        inverseTileMatrix[2][0],
        inverseTileMatrix[3][0]);
    pBufferData[Offset + 5] = FVector4f(
        inverseTileMatrix[0][1],
        inverseTileMatrix[1][1],
        inverseTileMatrix[2][1],
        inverseTileMatrix[3][1]);
    pBufferData[Offset + 6] = FVector4f(
        inverseTileMatrix[0][2],
        inverseTileMatrix[1][2],
        inverseTileMatrix[2][2],
        inverseTileMatrix[3][2]);
    pBufferData[Offset + 7] = FVector4f(
        inverseTileMatrix[0][3],
        inverseTileMatrix[1][3],
        inverseTileMatrix[2][3],
        inverseTileMatrix[3][3]);

    float visibility = pComponent->IsVisible() ? 1.0f : 0.0f;
    pBufferData[Offset + 8] = FVector4f(
        static_cast<float>(tileScale.X),
        static_cast<float>(tileScale.Y),
        static_cast<float>(tileScale.Z),
        visibility);

    pBufferData[Offset + 9] = FVector4f(
        static_cast<float>(tileRotation.X),
        static_cast<float>(tileRotation.Y),
        static_cast<float>(tileRotation.Z),
        static_cast<float>(tileRotation.W));
  }

  RHICmdList.UnlockBuffer(Buffer.Buffer);
}

void updatePerSplatData(
    FRHICommandListImmediate& RHICmdList,
    const TArray<const UCesiumGltfGaussianSplatComponent*>& Components,
    int32 ShCoefficientCount,
    int32 SplatCount,
    FNiagaraDataInterfaceProxyCesiumGaussianSplats& Proxy) {
  if (Components.IsEmpty()) {
    Proxy.PositionsBuffer.Release();
    Proxy.ScalesBuffer.Release();
    Proxy.OrientationsBuffer.Release();
    Proxy.ColorsBuffer.Release();
    Proxy.TileIndicesBuffer.Release();
    Proxy.SHNonZeroCoeffsBuffer.Release();
    Proxy.SplatSHDegreesBuffer.Release();
    return;
  }

  const int32 requiredPositionBytes = SplatCount * sizeof(FVector4f);
  const int32 requiredDegreesBytes = Components.Num() * 3 * sizeof(uint32);
  const int32 requiredShCoeffBytes = ShCoefficientCount * 2 * sizeof(float);

  if (Proxy.PositionsBuffer.NumBytes != requiredPositionBytes) {
    Proxy.PositionsBuffer.Release();
    Proxy.ScalesBuffer.Release();
    Proxy.OrientationsBuffer.Release();
    Proxy.ColorsBuffer.Release();
    Proxy.TileIndicesBuffer.Release();

    Proxy.PositionsBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Positions"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);
    Proxy.ScalesBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Scales"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);
    Proxy.OrientationsBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Orientations"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);
    Proxy.ColorsBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Colors"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);

    if (ShCoefficientCount > 0) {
      Proxy.SHNonZeroCoeffsBuffer.Initialize(
          RHICmdList,
          TEXT(
              "FNiagaraDataInterfaceProxyCesiumGaussianSplat_SHNonZeroCoeffsBuffer"),
          sizeof(FVector4f),
          ShCoefficientCount,
          EPixelFormat::PF_A32B32G32R32F,
          BUF_Static);
    }
    Proxy.TileIndicesBuffer.Initialize(
        RHICmdList,
        TEXT(
            "FNiagaraDataInterfaceProxyCesiumGaussianSplat_SplatIndicesBuffer"),
        sizeof(uint32),
        SplatCount,
        EPixelFormat::PF_R32_UINT,
        BUF_Static);
  }

  if (Proxy.SplatSHDegreesBuffer.NumBytes != requiredDegreesBytes) {
    Proxy.SplatSHDegreesBuffer.Release();
    Proxy.SplatSHDegreesBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_SplatSHDegrees"),
        sizeof(uint32),
        Components.Num() * 3,
        EPixelFormat::PF_R32_UINT,
        BUF_Static);
  }

  if (Proxy.SHNonZeroCoeffsBuffer.NumBytes != requiredShCoeffBytes) {
    Proxy.SHNonZeroCoeffsBuffer.Release();
    if (ShCoefficientCount > 0) {
      Proxy.SHNonZeroCoeffsBuffer.Initialize(
          RHICmdList,
          TEXT(
              "FNiagaraDataInterfaceProxyCesiumGaussianSplat_SHNonZeroCoeffsBuffer"),
          sizeof(FVector4f),
          ShCoefficientCount,
          EPixelFormat::PF_A32B32G32R32F,
          BUF_Static);
    }
  }

  float* pPositionsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
      Proxy.PositionsBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pScalesBuffer = static_cast<float*>(RHICmdList.LockBuffer(
      Proxy.ScalesBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pOrientationsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
      Proxy.OrientationsBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pColorsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
      Proxy.ColorsBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pSHNonZeroCoeffsBuffer =
      ShCoefficientCount > 0 ? static_cast<float*>(RHICmdList.LockBuffer(
                                   Proxy.SHNonZeroCoeffsBuffer.Buffer,
                                   0,
                                   ShCoefficientCount * 4 * sizeof(float),
                                   EResourceLockMode::RLM_WriteOnly))
                             : nullptr;
  uint32* pIndexBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
      Proxy.TileIndicesBuffer.Buffer,
      0,
      SplatCount * sizeof(uint32),
      EResourceLockMode::RLM_WriteOnly));
  uint32* pSHDegreesBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
      Proxy.SplatSHDegreesBuffer.Buffer,
      0,
      Components.Num() * sizeof(uint32) * 3,
      EResourceLockMode::RLM_WriteOnly));

  int32 coeffCountWritten = 0;
  int32 splatCountWritten = 0;
  for (int32 i = 0; i < Components.Num(); i++) {
    const UCesiumGltfGaussianSplatComponent* pComponent = Components[i];
    check(pComponent);

    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pPositionsBuffer + splatCountWritten * 4),
        pComponent->Data.Positions.GetData(),
        pComponent->Data.Positions.Num() * sizeof(float));
    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pScalesBuffer + splatCountWritten * 4),
        pComponent->Data.Scales.GetData(),
        pComponent->Data.Scales.Num() * sizeof(float));
    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pOrientationsBuffer + splatCountWritten * 4),
        pComponent->Data.Orientations.GetData(),
        pComponent->Data.Orientations.Num() * sizeof(float));
    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pColorsBuffer + splatCountWritten * 4),
        pComponent->Data.Colors.GetData(),
        pComponent->Data.Colors.Num() * sizeof(float));
    if (ShCoefficientCount > 0) {
      FPlatformMemory::Memcpy(
          reinterpret_cast<void*>(
              pSHNonZeroCoeffsBuffer + coeffCountWritten * 4),
          pComponent->Data.SphericalHarmonics.GetData(),
          pComponent->Data.SphericalHarmonics.Num() * sizeof(float));
    }
    for (int32 j = 0; j < pComponent->Data.NumSplats; j++) {
      pIndexBuffer[splatCountWritten + j] = static_cast<uint32>(i);
    }

    pSHDegreesBuffer[i * 3] =
        static_cast<uint32>(pComponent->Data.NumCoefficients);
    pSHDegreesBuffer[i * 3 + 1] = static_cast<uint32>(coeffCountWritten);
    pSHDegreesBuffer[i * 3 + 2] = static_cast<uint32>(splatCountWritten);

    splatCountWritten += pComponent->Data.NumSplats;
    coeffCountWritten +=
        pComponent->Data.NumSplats * pComponent->Data.NumCoefficients;
  }

  RHICmdList.UnlockBuffer(Proxy.PositionsBuffer.Buffer);
  RHICmdList.UnlockBuffer(Proxy.ScalesBuffer.Buffer);
  RHICmdList.UnlockBuffer(Proxy.OrientationsBuffer.Buffer);
  RHICmdList.UnlockBuffer(Proxy.ColorsBuffer.Buffer);
  if (ShCoefficientCount > 0) {
    RHICmdList.UnlockBuffer(Proxy.SHNonZeroCoeffsBuffer.Buffer);
  }
  RHICmdList.UnlockBuffer(Proxy.TileIndicesBuffer.Buffer);
  RHICmdList.UnlockBuffer(Proxy.SplatSHDegreesBuffer.Buffer);
}
} // namespace

UCesiumGaussianSplatDataInterface::UCesiumGaussianSplatDataInterface(
    const FObjectInitializer& Initializer)
    : UNiagaraDataInterface(Initializer) {
  this->Proxy = MakeUnique<FNiagaraDataInterfaceProxyCesiumGaussianSplats>();
}

#if WITH_EDITORONLY_DATA

void UCesiumGaussianSplatDataInterface::GetParameterDefinitionHLSL(
    const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    FString& OutHLSL) {
  UNiagaraDataInterface::GetParameterDefinitionHLSL(ParamInfo, OutHLSL);
  OutHLSL.Appendf(
      TEXT("Buffer<uint> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_TileIndices"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_TileTransforms"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_Positions"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_Scales"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_Orientations"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_Colors"));
  OutHLSL.Appendf(
      TEXT("Buffer<uint> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SplatSHDegrees"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SHNonZeroCoeffs"));
}

bool UCesiumGaussianSplatDataInterface::GetFunctionHLSL(
    const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
    int FunctionInstanceIndex,
    FString& OutHLSL) {
  if (UNiagaraDataInterface::GetFunctionHLSL(
          ParamInfo,
          FunctionInfo,
          FunctionInstanceIndex,
          OutHLSL)) {
    return true;
  }

  if (FunctionInfo.DefinitionName == *ComputeSplatFunctionName) {
    const FString Path = GetShaderSourceFilePath(
        "/Plugin/CesiumForUnreal/Private/CesiumGaussianSplatCompute.usf");
    if (Path.IsEmpty()) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "Can't find source file path for gaussian splat compute shader"));
      return false;
    }

    FString ShaderTemplate;
    FFileHelper::LoadFileToString(ShaderTemplate, *Path);

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("IndicesBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileIndices"))},
        {TEXT("TileTransformsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileTransforms"))},
        {TEXT("SHCoeffs"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SHNonZeroCoeffs"))},
        {TEXT("SHDegrees"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatSHDegrees"))},
        {TEXT("ScalesBuffer"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Scales"))},
        {TEXT("OrientationsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Orientations"))},
        {TEXT("ColorsBuffer"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Colors"))},
        {TEXT("PositionsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Positions"))}};

    OutHLSL += FString::Format(*ShaderTemplate, ArgsBounds);
  } else {
    return false;
  }

  return true;
}

bool UCesiumGaussianSplatDataInterface::AppendCompileHash(
    FNiagaraCompileHashVisitor* InVisitor) const {
  bool bSuccess = UNiagaraDataInterface::AppendCompileHash(InVisitor);
  bSuccess &= InVisitor->UpdateShaderParameters<FGaussianSplatShaderParams>();
  return bSuccess;
}

void UCesiumGaussianSplatDataInterface::GetFunctions(
    TArray<FNiagaraFunctionSignature>& OutFunctions) {
  FNiagaraFunctionSignature Sig;
  Sig.Name = *ComputeSplatFunctionName;
  Sig.Inputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition(GetClass()),
      TEXT("GaussianSplatNDI")));
  Sig.Inputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition::GetMatrix4Def(),
      TEXT("M_SystemLocalToWorld")));
  Sig.Inputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition::GetMatrix4Def(),
      TEXT("M_SystemWorldToLocal")));
  Sig.Inputs.Add(
      FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
  Sig.Inputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition::GetVec3Def(),
      TEXT("CameraPosition")));
  Sig.Outputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition::GetVec4Def(),
      TEXT("OutPosition")));
  Sig.Outputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition::GetColorDef(),
      TEXT("OutColor")));
  Sig.Outputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition::GetVec2Def(),
      TEXT("OutSpriteSize")));
  Sig.Outputs.Add(FNiagaraVariable(
      FNiagaraTypeDefinition::GetFloatDef(),
      TEXT("OutSpriteRotation")));
  Sig.bMemberFunction = true;
  Sig.bRequiresContext = false;
  OutFunctions.Add(Sig);
}
#endif

void UCesiumGaussianSplatDataInterface::BuildShaderParameters(
    FNiagaraShaderParametersBuilder& Builder) const {
  Builder.AddNestedStruct<FGaussianSplatShaderParams>();
}

void UCesiumGaussianSplatDataInterface::SetShaderParameters(
    const FNiagaraDataInterfaceSetShaderParametersContext& Context) const {
  FGaussianSplatShaderParams* Params =
      Context.GetParameterNestedStruct<FGaussianSplatShaderParams>();
  if (!Params) {
    return;
  }

  auto& DIProxy =
      Context.GetProxy<FNiagaraDataInterfaceProxyCesiumGaussianSplats>();
  Params->TileIndices =
      FNiagaraRenderer::GetSrvOrDefaultUInt(DIProxy.TileIndicesBuffer.SRV);
  Params->TileTransforms =
      FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.TileTransformsBuffer.SRV);
  Params->Positions =
      FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.PositionsBuffer.SRV);
  Params->Scales =
      FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.ScalesBuffer.SRV);
  Params->Orientations =
      FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.OrientationsBuffer.SRV);
  Params->Colors =
      FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.ColorsBuffer.SRV);
  Params->SHNonZeroCoeffs = FNiagaraRenderer::GetSrvOrDefaultFloat4(
      DIProxy.SHNonZeroCoeffsBuffer.SRV);
  Params->SplatSHDegrees =
      FNiagaraRenderer::GetSrvOrDefaultUInt(DIProxy.SplatSHDegreesBuffer.SRV);
}

void UCesiumGaussianSplatDataInterface::PostInitProperties() {
  UNiagaraDataInterface::PostInitProperties();

  if (HasAnyFlags(RF_ClassDefaultObject)) {
    ENiagaraTypeRegistryFlags DIFlags =
        ENiagaraTypeRegistryFlags::AllowAnyVariable |
        ENiagaraTypeRegistryFlags::AllowParameter;
    FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), DIFlags);
  }
}

bool UCesiumGaussianSplatDataInterface::InitPerInstanceData(
    void* PerInstanceData,
    FNiagaraSystemInstance* SystemInstance) {
  check(SystemInstance);
  FNDICesiumGaussianSplats_InstanceData* pInstData =
      new (PerInstanceData) FNDICesiumGaussianSplats_InstanceData();

  // By design, only one Niagara system for rendering splats should exist per
  // world. If this check fails, something went wrong.
  check(!this->_worldToProxyData.Contains(SystemInstance->GetWorld()));
  this->_worldToProxyData.Emplace(SystemInstance->GetWorld(), pInstData);
  return true;
}

void UCesiumGaussianSplatDataInterface::DestroyPerInstanceData(
    void* PerInstanceData,
    FNiagaraSystemInstance* SystemInstance) {
  check(SystemInstance);
  this->_worldToProxyData.Remove(SystemInstance->GetWorld());

  FNDICesiumGaussianSplats_InstanceData* pInstData =
      static_cast<FNDICesiumGaussianSplats_InstanceData*>(PerInstanceData);
  pInstData->~FNDICesiumGaussianSplats_InstanceData();
}

/**
 * When this function returns true, the per-instance data is destroyed and
 * reinitialized. (see NiagaraSystemInstance.cpp)
 *
 * Most DataInterface implementations return false unless something went wrong
 * (e.g. null pointers, uninitialized instance data), in which case it returns
 * true.
 */
bool UCesiumGaussianSplatDataInterface::PerInstanceTick(
    void* PerInstanceData,
    FNiagaraSystemInstance* SystemInstance,
    float DeltaSeconds) {
  check(SystemInstance);

  FNDICesiumGaussianSplats_InstanceData* pData =
      (FNDICesiumGaussianSplats_InstanceData*)PerInstanceData;
  UCesiumGaussianSplatSubsystem* pSplatSystem =
      UCesiumGaussianSplatSubsystem::Get();
  UWorld* pWorld = this->GetWorld();

  if (!pData || !IsValid(pSplatSystem) || !IsValid(pWorld)) {
    return true;
  }

  switch (this->GetResourceState(pWorld)) {
  case ResourceState::Invalid:
    return true;
  case ResourceState::ExecutingRenderCommand:
    // Skip early if render commands from a previous tick are still in progress,
    // or if there is otherwise nothing to be done.
    return false;
  case ResourceState::Dirty:
    break;
  case ResourceState::Idle:
  default:
    return false;
  }

  pData->reset();
  int32 shCoefficientCount = 0;

  // In PIE mode, components can belong to different worlds. This pre-counts
  // components to measure how much should actually be allocated.
  for (const UCesiumGltfGaussianSplatComponent* pSplatComponent :
       pSplatSystem->SplatComponents) {
    check(pSplatComponent);
    if (pSplatComponent->GetWorld() != pWorld ||
        pSplatComponent->IsBeingDestroyed()) {
      continue;
    }

    pData->Components.Add(pSplatComponent);
    pData->SplatCount += pSplatComponent->Data.NumSplats;
    shCoefficientCount +=
        pSplatComponent->Data.NumCoefficients * pSplatComponent->Data.NumSplats;
  }

  FNiagaraDataInterfaceProxyCesiumGaussianSplats* RT_Proxy =
      this->GetProxyAs<FNiagaraDataInterfaceProxyCesiumGaussianSplats>();

  if (this->_splatsDirty) {
    this->_splatsDirty = false;

    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatBuffers)
    ([RT_Proxy,
      &components = pData->Components,
      shCoefficientCount,
      splatCount = pData->SplatCount](FRHICommandListImmediate& RHICmdList) {
      updatePerSplatData(
          RHICmdList,
          components,
          shCoefficientCount,
          splatCount,
          *RT_Proxy);
    });
    pData->SplatsFence.emplace().BeginFence();
  }

  if (this->_tilesDirty) {
    this->_tilesDirty = false;

    ENQUEUE_RENDER_COMMAND(FUpdateCesiumGaussianSplatMatrices)
    ([RT_Proxy,
      &Components = pData->Components](FRHICommandListImmediate& RHICmdList) {
      updateTileTransforms(
          RHICmdList,
          Components,
          RT_Proxy->TileTransformsBuffer);
    });
    pData->MatricesFence.emplace().BeginFence();
  }
  UE_LOG(LogCesium, Warning, TEXT("Sent render commands"));
  return false;
}

int32 UCesiumGaussianSplatDataInterface::PerInstanceDataSize() const {
  return sizeof(FNDICesiumGaussianSplats_InstanceData);
}

bool UCesiumGaussianSplatDataInterface::CanExecuteOnTarget(
    ENiagaraSimTarget target) const {
  return target == ENiagaraSimTarget::GPUComputeSim;
}

void UCesiumGaussianSplatDataInterface::MarkDirty() {
  this->_splatsDirty = true;
  this->_tilesDirty = true;
}

void UCesiumGaussianSplatDataInterface::MarkTilesDirty() {
  this->_tilesDirty = true;
}

UCesiumGaussianSplatDataInterface::ResourceState
UCesiumGaussianSplatDataInterface::GetResourceState(UWorld* pWorld) const {
  if (!this->_worldToProxyData.Contains(pWorld)) {
    return ResourceState::Invalid;
  }

  const auto& matricesFence = this->_worldToProxyData[pWorld]->MatricesFence;
  const auto& splatsFence = this->_worldToProxyData[pWorld]->SplatsFence;

  if ((matricesFence && !matricesFence->IsFenceComplete()) ||
      (splatsFence && !splatsFence->IsFenceComplete())) {
    return ResourceState::ExecutingRenderCommand;
  }

  if (this->_tilesDirty || this->_splatsDirty) {
    return ResourceState::Dirty;
  }

  return ResourceState::Idle;
}

TSet<const UCesiumGltfGaussianSplatComponent*>
UCesiumGaussianSplatDataInterface::GetComponentsInUpdateForWorld(
    UWorld* pWorld) const {
  if (!this->_worldToProxyData.Contains(pWorld)) {
    return TSet<const UCesiumGltfGaussianSplatComponent*>();
  }
  FNDICesiumGaussianSplats_InstanceData* pInstanceData =
      this->_worldToProxyData.FindRef(pWorld);
  return TSet<const UCesiumGltfGaussianSplatComponent*>(
      pInstanceData->Components);
}

int32 UCesiumGaussianSplatDataInterface::GetSplatCountInUpdateForWorld(
    UWorld* pWorld) const {
  return this->_worldToProxyData.Contains(pWorld)
             ? this->_worldToProxyData.FindRef(pWorld)->SplatCount
             : 0;
}
