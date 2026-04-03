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

FNDICesiumGaussianSplats_InstanceData_RenderThread::
    ~FNDICesiumGaussianSplats_InstanceData_RenderThread() {
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
    check(IsValid(pComponent));

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
    FNDICesiumGaussianSplats_InstanceData_RenderThread& RT_Data) {
  if (Components.IsEmpty()) {
    RT_Data.PositionsBuffer.Release();
    RT_Data.ScalesBuffer.Release();
    RT_Data.OrientationsBuffer.Release();
    RT_Data.ColorsBuffer.Release();
    RT_Data.TileIndicesBuffer.Release();
    RT_Data.SHNonZeroCoeffsBuffer.Release();
    RT_Data.SplatSHDegreesBuffer.Release();
    return;
  }

  const int32 requiredPositionBytes = SplatCount * sizeof(FVector4f);
  const int32 requiredDegreesBytes = Components.Num() * 3 * sizeof(uint32);
  const int32 requiredShCoeffBytes = ShCoefficientCount * 2 * sizeof(float);

  if (RT_Data.PositionsBuffer.NumBytes != requiredPositionBytes) {
    RT_Data.PositionsBuffer.Release();
    RT_Data.ScalesBuffer.Release();
    RT_Data.OrientationsBuffer.Release();
    RT_Data.ColorsBuffer.Release();
    RT_Data.TileIndicesBuffer.Release();

    RT_Data.PositionsBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Positions"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);
    RT_Data.ScalesBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Scales"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);
    RT_Data.OrientationsBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Orientations"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);
    RT_Data.ColorsBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_Colors"),
        sizeof(FVector4f),
        SplatCount,
        EPixelFormat::PF_A32B32G32R32F,
        BUF_Static);

    if (ShCoefficientCount > 0) {
      RT_Data.SHNonZeroCoeffsBuffer.Initialize(
          RHICmdList,
          TEXT(
              "FNiagaraDataInterfaceProxyCesiumGaussianSplat_SHNonZeroCoeffsBuffer"),
          sizeof(FVector4f),
          ShCoefficientCount,
          EPixelFormat::PF_A32B32G32R32F,
          BUF_Static);
    }
    RT_Data.TileIndicesBuffer.Initialize(
        RHICmdList,
        TEXT(
            "FNiagaraDataInterfaceProxyCesiumGaussianSplat_SplatIndicesBuffer"),
        sizeof(uint32),
        SplatCount,
        EPixelFormat::PF_R32_UINT,
        BUF_Static);
  }

  if (RT_Data.SplatSHDegreesBuffer.NumBytes != requiredDegreesBytes) {
    RT_Data.SplatSHDegreesBuffer.Release();
    RT_Data.SplatSHDegreesBuffer.Initialize(
        RHICmdList,
        TEXT("FNiagaraDataInterfaceProxyCesiumGaussianSplat_SplatSHDegrees"),
        sizeof(uint32),
        Components.Num() * 3,
        EPixelFormat::PF_R32_UINT,
        BUF_Static);
  }

  if (RT_Data.SHNonZeroCoeffsBuffer.NumBytes != requiredShCoeffBytes) {
    RT_Data.SHNonZeroCoeffsBuffer.Release();
    if (ShCoefficientCount > 0) {
      RT_Data.SHNonZeroCoeffsBuffer.Initialize(
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
      RT_Data.PositionsBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pScalesBuffer = static_cast<float*>(RHICmdList.LockBuffer(
      RT_Data.ScalesBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pOrientationsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
      RT_Data.OrientationsBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pColorsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
      RT_Data.ColorsBuffer.Buffer,
      0,
      requiredPositionBytes,
      EResourceLockMode::RLM_WriteOnly));
  float* pSHNonZeroCoeffsBuffer =
      ShCoefficientCount > 0 ? static_cast<float*>(RHICmdList.LockBuffer(
                                   RT_Data.SHNonZeroCoeffsBuffer.Buffer,
                                   0,
                                   ShCoefficientCount * 4 * sizeof(float),
                                   EResourceLockMode::RLM_WriteOnly))
                             : nullptr;
  uint32* pIndexBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
      RT_Data.TileIndicesBuffer.Buffer,
      0,
      SplatCount * sizeof(uint32),
      EResourceLockMode::RLM_WriteOnly));
  uint32* pSHDegreesBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
      RT_Data.SplatSHDegreesBuffer.Buffer,
      0,
      Components.Num() * sizeof(uint32) * 3,
      EResourceLockMode::RLM_WriteOnly));

  int32 CoeffCountWritten = 0;
  int32 SplatCountWritten = 0;
  int32 CurrentIdx = 0;
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
      pIndexBuffer[SplatCountWritten + j] = static_cast<uint32>(CurrentIdx);
    }

    pSHDegreesBuffer[CurrentIdx * 3] =
        static_cast<uint32>(Component->Data.NumCoefficients);
    pSHDegreesBuffer[CurrentIdx * 3 + 1] =
        static_cast<uint32>(CoeffCountWritten);
    pSHDegreesBuffer[CurrentIdx * 3 + 2] =
        static_cast<uint32>(SplatCountWritten);

    SplatCountWritten += Component->Data.NumSplats;
    CoeffCountWritten +=
        Component->Data.NumSplats * Component->Data.NumCoefficients;
    CurrentIdx++;
  }

  RHICmdList.UnlockBuffer(RT_Data.PositionsBuffer.Buffer);
  RHICmdList.UnlockBuffer(RT_Data.ScalesBuffer.Buffer);
  RHICmdList.UnlockBuffer(RT_Data.OrientationsBuffer.Buffer);
  RHICmdList.UnlockBuffer(RT_Data.ColorsBuffer.Buffer);
  if (ShCoefficientCount > 0) {
    RHICmdList.UnlockBuffer(RT_Data.SHNonZeroCoeffsBuffer.Buffer);
  }
  RHICmdList.UnlockBuffer(RT_Data.TileIndicesBuffer.Buffer);
  RHICmdList.UnlockBuffer(RT_Data.SplatSHDegreesBuffer.Buffer);
}
} // namespace

struct FNiagaraDataInterfaceProxyCesiumGaussianSplats
    : public FNiagaraDataInterfaceProxy {
  virtual int32 PerInstanceDataPassedToRenderThreadSize() const override {
    return 0;
  }

  // List of proxy data for each system instance
  TMap<
      FNiagaraSystemInstanceID,
      FNDICesiumGaussianSplats_InstanceData_RenderThread>
      SystemInstancesToProxyData_RT;
};

UCesiumGaussianSplatDataInterface::UCesiumGaussianSplatDataInterface(
    const FObjectInitializer& Initializer)
    : UNiagaraDataInterface(Initializer) {
  this->Proxy =
      MakeUnique<FNiagaraDataInterfaceProxyCesiumGaussianSplats>(this);
}

BEGIN_SHADER_PARAMETER_STRUCT(FGaussianSplatShaderParams, )
SHADER_PARAMETER_SRV(Buffer<uint>, TileIndices)
SHADER_PARAMETER_SRV(Buffer<float4>, TileTransforms)
SHADER_PARAMETER_SRV(Buffer<float4>, Positions)
SHADER_PARAMETER_SRV(Buffer<float3>, Scales)
SHADER_PARAMETER_SRV(Buffer<float4>, Orientations)
SHADER_PARAMETER_SRV(Buffer<float4>, Colors)
SHADER_PARAMETER_SRV(Buffer<uint>, SplatSHDegrees)
SHADER_PARAMETER_SRV(Buffer<float3>, SHNonZeroCoeffs)
END_SHADER_PARAMETER_STRUCT()

#if WITH_EDITORONLY_DATA

void UCesiumGaussianSplatDataInterface::GetParameterDefinitionHLSL(
    const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    FString& OutHLSL) {
  UNiagaraDataInterface::GetParameterDefinitionHLSL(ParamInfo, OutHLSL);
  OutHLSL.Appendf(
      TEXT("uint %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SplatCount"));
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
        {TEXT("SplatCount"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatCount"))},
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
  if (FNDICesiumGaussianSplats_InstanceData_RenderThread* pInstanceData_RT =
          DIProxy.SystemInstancesToProxyData_RT.Find(
              Context.GetSystemInstanceID())) {
    Params->TileIndices = FNiagaraRenderer::GetSrvOrDefaultUInt(
        pInstanceData_RT->PositionsBuffer.SRV);
    Params->TileTransforms = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        pInstanceData_RT->TileTransformsBuffer.SRV);
    Params->Positions = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        pInstanceData_RT->PositionsBuffer.SRV);
    Params->Scales = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        pInstanceData_RT->ScalesBuffer.SRV);
    Params->Orientations = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        pInstanceData_RT->OrientationsBuffer.SRV);
    Params->Colors = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        pInstanceData_RT->ColorsBuffer.SRV);
    Params->SHNonZeroCoeffs = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        pInstanceData_RT->SHNonZeroCoeffsBuffer.SRV);
    Params->SplatSHDegrees = FNiagaraRenderer::GetSrvOrDefaultUInt(
        pInstanceData_RT->SplatSHDegreesBuffer.SRV);
  } else {
    Params->TileIndices = FNiagaraRenderer::GetDummyUIntBuffer();
    Params->TileTransforms = FNiagaraRenderer::GetDummyFloat4Buffer();
    Params->Positions = FNiagaraRenderer::GetDummyFloat4Buffer();
    Params->Scales = FNiagaraRenderer::GetDummyFloat4Buffer();
    Params->Orientations = FNiagaraRenderer::GetDummyFloat4Buffer();
    Params->Colors = FNiagaraRenderer::GetDummyFloat4Buffer();
    Params->SHNonZeroCoeffs = FNiagaraRenderer::GetDummyFloat4Buffer();
    Params->SplatSHDegrees = FNiagaraRenderer::GetDummyUIntBuffer();
  }
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
  return true;
}

void UCesiumGaussianSplatDataInterface::DestroyPerInstanceData(
    void* PerInstanceData,
    FNiagaraSystemInstance* SystemInstance) {
  FNiagaraDataInterfaceProxyCesiumGaussianSplats* RT_Proxy =
      GetProxyAs<FNiagaraDataInterfaceProxyCesiumGaussianSplats>();
  ENQUEUE_RENDER_COMMAND(FNiagaraDIDestroyInstanceData)
  ([RT_Proxy,
    InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& CmdList) {
    RT_Proxy->SystemInstancesToProxyData_RT.Remove(InstanceID);
  });
}

bool UCesiumGaussianSplatDataInterface::PerInstanceTick(
    void* PerInstanceData,
    FNiagaraSystemInstance* SystemInstance,
    float DeltaSeconds) {
  check(SystemInstance);
  if (!this->_matricesDirty && !this->_matricesDirty) {
    return false;
  }

  UCesiumGaussianSplatSubsystem* pSplatSystem =
      UCesiumGaussianSplatSubsystem::Get();
  UWorld* pWorld = this->GetWorld();
  if (!IsValid(pSplatSystem) || !IsValid(pWorld)) {
    return false;
  }

  TArray<const UCesiumGltfGaussianSplatComponent*> componentsInWorld;
  int32 shCoefficientCount = 0;
  int32 splatCount = 0;

  // In PIE mode, components can belong to different worlds; this pre-counts
  // components to measure how much should actually be allocated.
  for (const UCesiumGltfGaussianSplatComponent* pSplatComponent :
       pSplatSystem->SplatComponents) {
    check(pSplatComponent);
    if (pSplatComponent->GetWorld() != pWorld) {
      continue;
    }

    componentsInWorld.Add(pSplatComponent);
    shCoefficientCount +=
        pSplatComponent->Data.NumCoefficients * pSplatComponent->Data.NumSplats;
    splatCount += pSplatComponent->Data.NumSplats;
  }

  FNiagaraDataInterfaceProxyCesiumGaussianSplats* RT_Proxy =
      this->GetProxyAs<FNiagaraDataInterfaceProxyCesiumGaussianSplats>();

  if (this->_matricesDirty) {
    this->_matricesDirty = false;

    ENQUEUE_RENDER_COMMAND(FUpdateCesiumGaussianSplatMatrices)
    ([RT_Proxy,
      InstanceId = SystemInstance->GetId(),
      Components = componentsInWorld](FRHICommandListImmediate& RHICmdList) {
      FNDICesiumGaussianSplats_InstanceData_RenderThread* pTargetData =
          &RT_Proxy->SystemInstancesToProxyData_RT.FindOrAdd(InstanceId);
      check(pTargetData);
      updateTileTransforms(
          RHICmdList,
          Components,
          pTargetData->TileTransformsBuffer);
    });
  }

  if (this->_splatsDirty) {
    this->_splatsDirty = false;
    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatBuffers)
    ([RT_Proxy,
      InstanceId = SystemInstance->GetId(),
      Components = componentsInWorld,
      shCoefficientCount,
      splatCount](FRHICommandListImmediate& RHICmdList) {
      FNDICesiumGaussianSplats_InstanceData_RenderThread* pTargetData =
          &RT_Proxy->SystemInstancesToProxyData_RT.FindOrAdd(InstanceId);
      check(pTargetData);
      updatePerSplatData(
          RHICmdList,
          Components,
          shCoefficientCount,
          splatCount,
          *pTargetData);
    });
  }

  return true;
}

int32 UCesiumGaussianSplatDataInterface::PerInstanceDataSize() const {
  return 0;
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
