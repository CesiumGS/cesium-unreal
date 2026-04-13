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
#include "RenderCommandFence.h"
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
  const int32 VectorsPerComponent = 10;

  const int32 totalVectorCount = Components.Num() * VectorsPerComponent;
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

    const int32 Offset = i * VectorsPerComponent;

    glm::mat4 TileMatrix(pComponent->GetMatrix());
    const FTransform& ComponentToWorld = pComponent->GetComponentToWorld();
    FVector TileScale = ComponentToWorld.GetScale3D();
    FQuat TileRotation = ComponentToWorld.GetRotation();
    TileRotation.Normalize();

    // Write in row-order for easy access in HLSL.
    pBufferData[Offset + 0] = FVector4f(
        TileMatrix[0][0],
        TileMatrix[1][0],
        TileMatrix[2][0],
        TileMatrix[3][0]);
    pBufferData[Offset + 1] = FVector4f(
        TileMatrix[0][1],
        TileMatrix[1][1],
        TileMatrix[2][1],
        TileMatrix[3][1]);
    pBufferData[Offset + 2] = FVector4f(
        TileMatrix[0][2],
        TileMatrix[1][2],
        TileMatrix[2][2],
        TileMatrix[3][2]);
    pBufferData[Offset + 3] = FVector4f(
        TileMatrix[0][3],
        TileMatrix[1][3],
        TileMatrix[2][3],
        TileMatrix[3][3]);

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
        static_cast<float>(TileScale.X),
        static_cast<float>(TileScale.Y),
        static_cast<float>(TileScale.Z),
        visibility);

    pBufferData[Offset + 9] = FVector4f(
        static_cast<float>(TileRotation.X),
        static_cast<float>(TileRotation.Y),
        static_cast<float>(TileRotation.Z),
        static_cast<float>(TileRotation.W));
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

  int32 CoeffCountWritten = 0;
  int32 SplatCountWritten = 0;
  for (int32 i = 0; i < Components.Num(); i++) {
    const UCesiumGltfGaussianSplatComponent* Component = Components[i];
    check(Component);

    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pPositionsBuffer + SplatCountWritten * 4),
        Component->Data.Positions.GetData(),
        Component->Data.Positions.Num() * sizeof(float));
    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pScalesBuffer + SplatCountWritten * 4),
        Component->Data.Scales.GetData(),
        Component->Data.Scales.Num() * sizeof(float));
    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pOrientationsBuffer + SplatCountWritten * 4),
        Component->Data.Orientations.GetData(),
        Component->Data.Orientations.Num() * sizeof(float));
    FPlatformMemory::Memcpy(
        reinterpret_cast<void*>(pColorsBuffer + SplatCountWritten * 4),
        Component->Data.Colors.GetData(),
        Component->Data.Colors.Num() * sizeof(float));
    if (ShCoefficientCount > 0) {
      FPlatformMemory::Memcpy(
          reinterpret_cast<void*>(
              pSHNonZeroCoeffsBuffer + CoeffCountWritten * 4),
          Component->Data.SphericalHarmonics.GetData(),
          Component->Data.SphericalHarmonics.Num() * sizeof(float));
    }
    for (int32 j = 0; j < Component->Data.NumSplats; j++) {
      pIndexBuffer[SplatCountWritten + j] = static_cast<uint32>(i);
    }

    pSHDegreesBuffer[i * 3] =
        static_cast<uint32>(Component->Data.NumCoefficients);
    pSHDegreesBuffer[i * 3 + 1] = static_cast<uint32>(CoeffCountWritten);
    pSHDegreesBuffer[i * 3 + 2] = static_cast<uint32>(SplatCountWritten);

    SplatCountWritten += Component->Data.NumSplats;
    CoeffCountWritten +=
        Component->Data.NumSplats * Component->Data.NumCoefficients;
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

  check(!this->_systemInstancesToProxyData_GT.Contains(
      SystemInstance->GetWorld()));
  this->_systemInstancesToProxyData_GT.Emplace(
      SystemInstance->GetWorld(),
      pInstData);
  return true;
}

void UCesiumGaussianSplatDataInterface::DestroyPerInstanceData(
    void* PerInstanceData,
    FNiagaraSystemInstance* SystemInstance) {
  this->_systemInstancesToProxyData_GT.Remove(SystemInstance->GetWorld());

  FNDICesiumGaussianSplats_InstanceData* pInstData =
      static_cast<FNDICesiumGaussianSplats_InstanceData*>(PerInstanceData);
  pInstData->~FNDICesiumGaussianSplats_InstanceData();
}

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

  if (!this->_matricesDirty && !this->_splatsDirty) {
    return true;
  }

  bool isUpdatingMatrices = pData->UpdateMatricesFence &&
                            !pData->UpdateMatricesFence->IsFenceComplete();
  bool isUpdatingSplats =
      pData->UpdateSplatsFence && !pData->UpdateSplatsFence->IsFenceComplete();

  if (isUpdatingMatrices && isUpdatingSplats) {
    // Skip early if the render commands from a previous Tick are still in
    // progress.
    return true;
  }

  TArray<const UCesiumGltfGaussianSplatComponent*> Components;
  int32 ShCoefficientCount = 0;
  int32 SplatCount = 0;

  // In PIE mode, components can belong to different worlds; this pre-counts
  // components to measure how much should actually be allocated.
  for (const UCesiumGltfGaussianSplatComponent* pSplatComponent :
       pSplatSystem->SplatComponents) {
    check(pSplatComponent);
    if (pSplatComponent->GetWorld() != pWorld) {
      continue;
    }

    Components.Add(pSplatComponent);
    ShCoefficientCount +=
        pSplatComponent->Data.NumCoefficients * pSplatComponent->Data.NumSplats;
    SplatCount += pSplatComponent->Data.NumSplats;
  }

  FNiagaraDataInterfaceProxyCesiumGaussianSplats* RT_Proxy =
      this->GetProxyAs<FNiagaraDataInterfaceProxyCesiumGaussianSplats>();
  if (this->_matricesDirty && !isUpdatingMatrices) {
    this->_matricesDirty = false;

    pData->UpdateMatricesFence.reset();
    ENQUEUE_RENDER_COMMAND(FUpdateCesiumGaussianSplatMatrices)
    ([RT_Proxy, InstanceId = SystemInstance->GetId(), Components](
         FRHICommandListImmediate& RHICmdList) {
      updateTileTransforms(
          RHICmdList,
          Components,
          RT_Proxy->TileTransformsBuffer);
    });
    pData->UpdateMatricesFence.emplace().BeginFence();
  }

  if (this->_splatsDirty && !isUpdatingSplats) {
    this->_splatsDirty = false;

    pData->UpdateSplatsFence.reset();
    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatBuffers)
    ([RT_Proxy,
      InstanceId = SystemInstance->GetId(),
      Components,
      ShCoefficientCount,
      SplatCount](FRHICommandListImmediate& RHICmdList) {
      updatePerSplatData(
          RHICmdList,
          Components,
          ShCoefficientCount,
          SplatCount,
          *RT_Proxy);
    });
    pData->UpdateSplatsFence.emplace().BeginFence();
  }

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
  this->_matricesDirty = true;
}

void UCesiumGaussianSplatDataInterface::MarkMatricesDirty() {
  this->_matricesDirty = true;
}

bool UCesiumGaussianSplatDataInterface::isUpdatingForWorld(
    UWorld* pWorld) const {
  if (!this->_systemInstancesToProxyData_GT.Contains(pWorld))
    return false;

  const auto& updateMatricesFence =
      this->_systemInstancesToProxyData_GT[pWorld]->UpdateMatricesFence;
  const auto& updateSplatsFence =
      this->_systemInstancesToProxyData_GT[pWorld]->UpdateSplatsFence;

  return (updateMatricesFence && !updateMatricesFence->IsFenceComplete()) ||
         (updateSplatsFence && !updateSplatsFence->IsFenceComplete());
}
