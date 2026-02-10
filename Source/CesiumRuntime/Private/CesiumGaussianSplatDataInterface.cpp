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
#include "RHICommandList.h"
#include "ShaderCore.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

const FString ComputeSplatFunctionName = TEXT("ComputeSplat");

namespace {
void releaseIfNonEmpty(FReadBuffer& buffer) {
  if (buffer.NumBytes > 0) {
    buffer.Release();
  }
}

void updateTileTransforms(
    FRHICommandListImmediate& RHICmdList,
    TArray<UCesiumGltfGaussianSplatComponent*>& Components,
    FReadBuffer& Buffer) {
  releaseIfNonEmpty(Buffer);
  if (Components.IsEmpty()) {
    // Will crash if we try to allocate a buffer of size 0
    return;
  }

  // Each tile corresponds to six vectors:
  // 0: Tile transform row 0
  // 1: Tile transform row 1
  // 2: Tile transform row 2
  // 3: Tile transform row 3
  // 4: Tile scale (xyz), visibility (w)
  // 5: Tile rotation
  const int32 vectorsPerComponent = 6;

  Buffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_TileTransformsBuffer"),
      sizeof(FVector4f),
      Components.Num() * vectorsPerComponent,
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  FVector4f* pTransformData =
      reinterpret_cast<FVector4f*>(RHICmdList.LockBuffer(
          Buffer.Buffer,
          0,
          Components.Num() * sizeof(FVector4f) * vectorsPerComponent,
          EResourceLockMode::RLM_WriteOnly));

  int32 offset = 0;
  for (int32 i = 0; i < Components.Num(); i++) {
    UCesiumGltfGaussianSplatComponent* pComponent = Components[i];
    check(pComponent);

    glm::mat4 tileMatrix(pComponent->GetMatrix());
    const FTransform& ComponentToWorld = pComponent->GetComponentToWorld();
    FVector tileScale = ComponentToWorld.GetScale3D();
    FQuat tileRotation = ComponentToWorld.GetRotation();
    tileRotation.Normalize();

    // Write in row-order for easy access in HLSL.
    pTransformData[offset] = FVector4f(
        tileMatrix[0][0],
        tileMatrix[1][0],
        tileMatrix[2][0],
        tileMatrix[3][0]);
    pTransformData[offset + 1] = FVector4f(
        tileMatrix[0][1],
        tileMatrix[1][1],
        tileMatrix[2][1],
        tileMatrix[3][1]);
    pTransformData[offset + 2] = FVector4f(
        tileMatrix[0][2],
        tileMatrix[1][2],
        tileMatrix[2][2],
        tileMatrix[3][2]);
    pTransformData[offset + 3] = FVector4f(
        tileMatrix[0][3],
        tileMatrix[1][3],
        tileMatrix[2][3],
        tileMatrix[3][3]);

    float visibility = Components[i]->IsVisible() ? 1.0f : 0.0f;
    pTransformData[offset + 4] = FVector4f(
        static_cast<float>(tileScale.X),
        static_cast<float>(tileScale.Y),
        static_cast<float>(tileScale.Z),
        visibility);

    pTransformData[offset + 5] = FVector4f(
        static_cast<float>(tileRotation.X),
        static_cast<float>(tileRotation.Y),
        static_cast<float>(tileRotation.Z),
        static_cast<float>(tileRotation.W));
    offset += vectorsPerComponent;
  }

  RHICmdList.UnlockBuffer(Buffer.Buffer);
}

} // namespace

FNDIGaussianSplatProxy::FNDIGaussianSplatProxy(
    UCesiumGaussianSplatDataInterface* InOwner)
    : Owner(InOwner) {}

void FNDIGaussianSplatProxy::UploadToGPU(
    UCesiumGaussianSplatSubsystem* SplatSystem) {
  if (!this->Owner || !IsValid(SplatSystem)) {
    return;
  }

  if (this->bMatricesNeedUpdate) {
    this->bMatricesNeedUpdate = false;

    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatMatrices)
    ([this, SplatSystem](FRHICommandListImmediate& RHICmdList) {
      if (!IsValid(SplatSystem))
        return;

      FScopeLock ScopeLock(&this->BufferLock);
      updateTileTransforms(
          RHICmdList,
          SplatSystem->SplatComponents,
          this->TileTransformsBuffer);
    });
  }

  if (!this->bNeedsUpdate) {
    return;
  }

  this->bNeedsUpdate = false;

  ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatBuffers)
  ([this, SplatSystem](FRHICommandListImmediate& RHICmdList) {
    if (!IsValid(SplatSystem))
      return;

    FScopeLock ScopeLock(&this->BufferLock);

    const int32 NumSplats = SplatSystem->GetNumSplats();

    int32 TotalCoeffsCount = 0;
    for (const UCesiumGltfGaussianSplatComponent* SplatComponent :
         SplatSystem->SplatComponents) {
      check(SplatComponent);
      TotalCoeffsCount +=
          SplatComponent->NumCoefficients * SplatComponent->NumSplats;
    }

    const int32 expectedPositionBytes = NumSplats * 4 * sizeof(float);
    if (this->ColorsBuffer.NumBytes != expectedPositionBytes) {
      releaseIfNonEmpty(this->TileIndicesBuffer);
      releaseIfNonEmpty(this->PositionsBuffer);
      releaseIfNonEmpty(this->ScalesBuffer);
      releaseIfNonEmpty(this->RotationsBuffer);
      releaseIfNonEmpty(this->ColorsBuffer);
      releaseIfNonEmpty(this->SHNonZeroCoeffsBuffer);
      releaseIfNonEmpty(this->SplatSHDegreesBuffer);

      if (SplatSystem->SplatComponents.IsEmpty()) {
        return;
      }

      if (expectedPositionBytes > 0) {
        this->TileIndicesBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_TileIndicesBuffer"),
            sizeof(uint32),
            NumSplats,
            EPixelFormat::PF_R32_UINT,
            BUF_Static);
        this->PositionsBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_Positions"),
            sizeof(FVector4f),
            NumSplats,
            EPixelFormat::PF_A32B32G32R32F,
            BUF_Static);
        this->ScalesBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_Scales"),
            sizeof(FVector4f),
            NumSplats,
            EPixelFormat::PF_A32B32G32R32F,
            BUF_Static);
        this->RotationsBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_Rotations"),
            sizeof(FVector4f),
            NumSplats,
            EPixelFormat::PF_A32B32G32R32F,
            BUF_Static);
        this->ColorsBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_Colors"),
            sizeof(FVector4f),
            NumSplats,
            EPixelFormat::PF_A32B32G32R32F,
            BUF_Static);

        if (TotalCoeffsCount > 0) {
          this->SHNonZeroCoeffsBuffer.Initialize(
              RHICmdList,
              TEXT("FNDIGaussianSplatProxy_SHNonZeroCoeffsBuffer"),
              sizeof(FVector4f),
              TotalCoeffsCount,
              EPixelFormat::PF_A32B32G32R32F,
              BUF_Static);
        }

        this->SplatSHDegreesBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_SplatSHDegrees"),
            sizeof(uint32),
            SplatSystem->SplatComponents.Num() * 3,
            EPixelFormat::PF_R32_UINT,
            BUF_Static);

        uint32* pIndexData = static_cast<uint32*>(RHICmdList.LockBuffer(
            this->TileIndicesBuffer.Buffer,
            0,
            NumSplats * sizeof(uint32),
            EResourceLockMode::RLM_WriteOnly));
        float* pPositionData = static_cast<float*>(RHICmdList.LockBuffer(
            this->PositionsBuffer.Buffer,
            0,
            expectedPositionBytes,
            EResourceLockMode::RLM_WriteOnly));
        float* pScaleData = static_cast<float*>(RHICmdList.LockBuffer(
            this->ScalesBuffer.Buffer,
            0,
            expectedPositionBytes,
            EResourceLockMode::RLM_WriteOnly));
        float* pRotationData = static_cast<float*>(RHICmdList.LockBuffer(
            this->RotationsBuffer.Buffer,
            0,
            NumSplats * 4 * sizeof(float),
            EResourceLockMode::RLM_WriteOnly));
        float* pColorData = static_cast<float*>(RHICmdList.LockBuffer(
            this->ColorsBuffer.Buffer,
            0,
            NumSplats * 4 * sizeof(float),
            EResourceLockMode::RLM_WriteOnly));
        float* pSHNonZeroCoeffsData =
            TotalCoeffsCount > 0 ? static_cast<float*>(RHICmdList.LockBuffer(
                                       this->SHNonZeroCoeffsBuffer.Buffer,
                                       0,
                                       TotalCoeffsCount * 4 * sizeof(float),
                                       EResourceLockMode::RLM_WriteOnly))
                                 : nullptr;
        uint32* pSHDegreesData = static_cast<uint32*>(RHICmdList.LockBuffer(
            this->SplatSHDegreesBuffer.Buffer,
            0,
            SplatSystem->SplatComponents.Num() * sizeof(uint32) * 3,
            EResourceLockMode::RLM_WriteOnly));

        int32 CoeffCountWritten = 0;
        int32 SplatCountWritten = 0;
        for (int32 i = 0; i < SplatSystem->SplatComponents.Num(); i++) {
          UCesiumGltfGaussianSplatComponent* Component =
              SplatSystem->SplatComponents[i];
          check(Component);

          for (int32 j = 0; j < Component->NumSplats; j++) {
            pIndexData[SplatCountWritten + j] = static_cast<uint32>(i);
          }

          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(pPositionData + SplatCountWritten * 4),
              Component->Positions.GetData(),
              Component->Positions.Num() * sizeof(float));
          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(pScaleData + SplatCountWritten * 4),
              Component->Scales.GetData(),
              Component->Scales.Num() * sizeof(float));
          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(pRotationData + SplatCountWritten * 4),
              Component->Rotations.GetData(),
              Component->Rotations.Num() * sizeof(float));
          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(pColorData + SplatCountWritten * 4),
              Component->Colors.GetData(),
              Component->Colors.Num() * sizeof(float));
          if (TotalCoeffsCount > 0) {
            FPlatformMemory::Memcpy(
                reinterpret_cast<void*>(
                    pSHNonZeroCoeffsData + CoeffCountWritten * 4),
                Component->SphericalHarmonics.GetData(),
                Component->SphericalHarmonics.Num() * sizeof(float));
          }

          pSHDegreesData[i * 3] =
              static_cast<uint32>(Component->NumCoefficients);
          pSHDegreesData[i * 3 + 1] = static_cast<uint32>(CoeffCountWritten);
          pSHDegreesData[i * 3 + 2] = static_cast<uint32>(SplatCountWritten);

          SplatCountWritten += Component->NumSplats;
          CoeffCountWritten +=
              Component->NumSplats * Component->NumCoefficients;
        }

        RHICmdList.UnlockBuffer(this->TileIndicesBuffer.Buffer);
        RHICmdList.UnlockBuffer(this->ColorsBuffer.Buffer);
        if (TotalCoeffsCount > 0) {
          RHICmdList.UnlockBuffer(this->SHNonZeroCoeffsBuffer.Buffer);
        }
        RHICmdList.UnlockBuffer(this->SplatSHDegreesBuffer.Buffer);
      }
    }
  });
}

UCesiumGaussianSplatDataInterface::UCesiumGaussianSplatDataInterface(
    const FObjectInitializer& Initializer)
    : UNiagaraDataInterface(Initializer) {
  this->Proxy = MakeUnique<FNDIGaussianSplatProxy>(this);
}

#if WITH_EDITORONLY_DATA
void UCesiumGaussianSplatDataInterface::GetParameterDefinitionHLSL(
    const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
    FString& OutHLSL) {
  UNiagaraDataInterface::GetParameterDefinitionHLSL(ParamInfo, OutHLSL);

  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_TileTransforms"));
  OutHLSL.Appendf(
      TEXT("Buffer<uint> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_TileIndices"));
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
      TEXT("_Rotations"));
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
        {TEXT("TileTransformsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileTransforms"))},
        {TEXT("TileIndicesBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileIndices"))},
        {TEXT("PositionsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Positions"))},
        {TEXT("ScalesBuffer"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Scales"))},
        {TEXT("RotationsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Rotations"))},
        {TEXT("ColorsBuffer"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Colors"))},
        {TEXT("SHCoeffs"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SHNonZeroCoeffs"))},
        {TEXT("SHDegrees"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatSHDegrees"))}};

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
  FNDIGaussianSplatProxy& DIProxy = Context.GetProxy<FNDIGaussianSplatProxy>();

  if (Params) {
    UCesiumGaussianSplatSubsystem* SplatSystem = this->GetSubsystem();

    DIProxy.UploadToGPU(SplatSystem);

    Params->TileTransforms = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.TileTransformsBuffer.SRV);
    Params->TileIndices =
        FNiagaraRenderer::GetSrvOrDefaultUInt(DIProxy.TileIndicesBuffer.SRV);
    Params->Positions =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.PositionsBuffer.SRV);
    Params->Scales =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.ScalesBuffer.SRV);
    Params->Rotations =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.RotationsBuffer.SRV);
    Params->Colors =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.ColorsBuffer.SRV);
    Params->SHNonZeroCoeffs = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.SHNonZeroCoeffsBuffer.SRV);
    Params->SplatSHDegrees =
        FNiagaraRenderer::GetSrvOrDefaultUInt(DIProxy.SplatSHDegreesBuffer.SRV);
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

bool UCesiumGaussianSplatDataInterface::CanExecuteOnTarget(
    ENiagaraSimTarget target) const {
  return target == ENiagaraSimTarget::GPUComputeSim;
}

void UCesiumGaussianSplatDataInterface::Refresh() {
  this->GetProxyAs<FNDIGaussianSplatProxy>()->bNeedsUpdate = true;
  this->GetProxyAs<FNDIGaussianSplatProxy>()->bMatricesNeedUpdate = true;
}

void UCesiumGaussianSplatDataInterface::RefreshMatrices() {
  this->GetProxyAs<FNDIGaussianSplatProxy>()->bMatricesNeedUpdate = true;
}

FScopeLock UCesiumGaussianSplatDataInterface::LockGaussianBuffers() {
  return FScopeLock(&this->GetProxyAs<FNDIGaussianSplatProxy>()->BufferLock);
}

UCesiumGaussianSplatSubsystem*
UCesiumGaussianSplatDataInterface::GetSubsystem() const {
  if (!IsValid(GEngine)) {
    return nullptr;
  }

  return GEngine->GetEngineSubsystem<UCesiumGaussianSplatSubsystem>();
}
