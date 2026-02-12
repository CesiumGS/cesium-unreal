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
void UploadTileTransforms(
    UWorld* InWorld,
    FRHICommandListImmediate& RHICmdList,
    TArray<UCesiumGltfGaussianSplatComponent*>& Components,
    FReadBuffer& Buffer) {
  if (Buffer.NumBytes > 0) {
    Buffer.Release();
  }

  int32 NumComponents = 0;
  for (int32 i = 0; i < Components.Num(); i++) {
    if (IsValid(Components[i]) && Components[i]->GetWorld() == InWorld) {
      NumComponents++;
    }
  }

  if (NumComponents == 0) {
    // Will crash if we try to allocate a buffer of size 0
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

  Buffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_TileTransformsBuffer"),
      sizeof(FVector4f),
      NumComponents * VectorsPerComponent,
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  FVector4f* pBufferData = static_cast<FVector4f*>(RHICmdList.LockBuffer(
      Buffer.Buffer,
      0,
      NumComponents * sizeof(FVector4f) * VectorsPerComponent,
      EResourceLockMode::RLM_WriteOnly));

  int32 WrittenComponents = 0;
  for (int32 i = 0; i < Components.Num(); i++) {
    UCesiumGltfGaussianSplatComponent* pComponent = Components[i];
    check(pComponent);
    if (pComponent->GetWorld() != InWorld) {
      continue;
    }

    const int32 Offset = WrittenComponents * VectorsPerComponent;

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

    WrittenComponents++;
  }

  RHICmdList.UnlockBuffer(Buffer.Buffer);
}
} // namespace

FNDIGaussianSplatProxy::FNDIGaussianSplatProxy(
    UCesiumGaussianSplatDataInterface* InOwner)
    : Owner(InOwner) {}

void FNDIGaussianSplatProxy::UploadToGPU(
    UCesiumGaussianSplatSubsystem* SplatSystem) {
  if (this->Owner == nullptr || !IsValid(SplatSystem)) {
    return;
  }

  UWorld* World = this->Owner->GetWorld();

  if (this->bMatricesNeedUpdate) {
    this->bMatricesNeedUpdate = false;

    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatMatrices)
    ([this, SplatSystem, World](FRHICommandListImmediate& RHICmdList) {
      FScopeLock ScopeLock(&this->BufferLock);
      UploadTileTransforms(
          World,
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
  ([this, SplatSystem, World](FRHICommandListImmediate& RHICmdList) {
    FScopeLock ScopeLock(&this->BufferLock);

    int32 TotalCoeffsCount = 0;
    int32 TotalSplatComponentsCount = 0;
    int32 NumSplats = 0;
    for (const UCesiumGltfGaussianSplatComponent* SplatComponent :
         SplatSystem->SplatComponents) {
      check(SplatComponent);
      if (SplatComponent->GetWorld() != World) {
        continue;
      }
      TotalCoeffsCount +=
          SplatComponent->Data.NumCoefficients * SplatComponent->Data.NumSplats;
      NumSplats += SplatComponent->Data.NumSplats;
      TotalSplatComponentsCount++;
    }

    const int32 ExpectedPosBytes = NumSplats * 4 * sizeof(float);
    if (this->PositionsBuffer.NumBytes != ExpectedPosBytes) {
      if (this->PositionsBuffer.NumBytes > 0) {
        this->PositionsBuffer.Release();
        this->ScalesBuffer.Release();
        this->OrientationsBuffer.Release();
        this->ColorsBuffer.Release();
        this->SHNonZeroCoeffsBuffer.Release();
        this->SplatSHDegreesBuffer.Release();
        this->SplatIndicesBuffer.Release();
      }

      if (TotalSplatComponentsCount == 0) {
        return;
      }

      if (ExpectedPosBytes > 0) {
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
        this->OrientationsBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_Orientations"),
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
        this->SplatIndicesBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_SplatIndicesBuffer"),
            sizeof(uint32),
            NumSplats,
            EPixelFormat::PF_R32_UINT,
            BUF_Static);
        this->SplatSHDegreesBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_SplatSHDegrees"),
            sizeof(uint32),
            TotalSplatComponentsCount * 3,
            EPixelFormat::PF_R32_UINT,
            BUF_Static);
      }
    }

    if (ExpectedPosBytes > 0) {
      {
        float* PositionsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
            this->PositionsBuffer.Buffer,
            0,
            ExpectedPosBytes,
            EResourceLockMode::RLM_WriteOnly));
        float* ScalesBuffer = static_cast<float*>(RHICmdList.LockBuffer(
            this->ScalesBuffer.Buffer,
            0,
            ExpectedPosBytes,
            EResourceLockMode::RLM_WriteOnly));
        float* OrientationsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
            this->OrientationsBuffer.Buffer,
            0,
            NumSplats * 4 * sizeof(float),
            EResourceLockMode::RLM_WriteOnly));
        float* ColorsBuffer = static_cast<float*>(RHICmdList.LockBuffer(
            this->ColorsBuffer.Buffer,
            0,
            NumSplats * 4 * sizeof(float),
            EResourceLockMode::RLM_WriteOnly));
        float* SHNonZeroCoeffsBuffer =
            TotalCoeffsCount > 0 ? static_cast<float*>(RHICmdList.LockBuffer(
                                       this->SHNonZeroCoeffsBuffer.Buffer,
                                       0,
                                       TotalCoeffsCount * 4 * sizeof(float),
                                       EResourceLockMode::RLM_WriteOnly))
                                 : nullptr;
        uint32* IndexBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
            this->SplatIndicesBuffer.Buffer,
            0,
            NumSplats * sizeof(uint32),
            EResourceLockMode::RLM_WriteOnly));
        uint32* SHDegreesBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
            this->SplatSHDegreesBuffer.Buffer,
            0,
            TotalSplatComponentsCount * sizeof(uint32) * 3,
            EResourceLockMode::RLM_WriteOnly));

        int32 CoeffCountWritten = 0;
        int32 SplatCountWritten = 0;
        int32 CurrentIdx = 0;
        for (int32 i = 0; i < SplatSystem->SplatComponents.Num(); i++) {
          UCesiumGltfGaussianSplatComponent* Component =
              SplatSystem->SplatComponents[i];
          check(Component);
          if (Component->GetWorld() != World) {
            continue;
          }

          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(PositionsBuffer + SplatCountWritten * 4),
              Component->Data.Positions.GetData(),
              Component->Data.Positions.Num() * sizeof(float));
          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(ScalesBuffer + SplatCountWritten * 4),
              Component->Data.Scales.GetData(),
              Component->Data.Scales.Num() * sizeof(float));
          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(
                  OrientationsBuffer + SplatCountWritten * 4),
              Component->Data.Orientations.GetData(),
              Component->Data.Orientations.Num() * sizeof(float));
          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(ColorsBuffer + SplatCountWritten * 4),
              Component->Data.Colors.GetData(),
              Component->Data.Colors.Num() * sizeof(float));
          if (TotalCoeffsCount > 0) {
            FPlatformMemory::Memcpy(
                reinterpret_cast<void*>(
                    SHNonZeroCoeffsBuffer + CoeffCountWritten * 4),
                Component->Data.SphericalHarmonics.GetData(),
                Component->Data.SphericalHarmonics.Num() * sizeof(float));
          }
          for (int32 j = 0; j < Component->Data.NumSplats; j++) {
            IndexBuffer[SplatCountWritten + j] =
                static_cast<uint32>(CurrentIdx);
          }

          SHDegreesBuffer[CurrentIdx * 3] =
              static_cast<uint32>(Component->Data.NumCoefficients);
          SHDegreesBuffer[CurrentIdx * 3 + 1] =
              static_cast<uint32>(CoeffCountWritten);
          SHDegreesBuffer[CurrentIdx * 3 + 2] =
              static_cast<uint32>(SplatCountWritten);

          SplatCountWritten += Component->Data.NumSplats;
          CoeffCountWritten +=
              Component->Data.NumSplats * Component->Data.NumCoefficients;
          CurrentIdx++;
        }

        RHICmdList.UnlockBuffer(this->PositionsBuffer.Buffer);
        RHICmdList.UnlockBuffer(this->ScalesBuffer.Buffer);
        RHICmdList.UnlockBuffer(this->OrientationsBuffer.Buffer);
        RHICmdList.UnlockBuffer(this->ColorsBuffer.Buffer);
        if (TotalCoeffsCount > 0) {
          RHICmdList.UnlockBuffer(this->SHNonZeroCoeffsBuffer.Buffer);
        }
        RHICmdList.UnlockBuffer(this->SplatIndicesBuffer.Buffer);
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
      TEXT("int %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SplatsCount"));
  OutHLSL.Appendf(
      TEXT("Buffer<uint> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SplatIndices"));
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
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
        {TEXT("TileTransformsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileTransforms"))},
        {TEXT("SHCoeffs"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SHNonZeroCoeffs"))},
        {TEXT("SHDegrees"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatSHDegrees"))},
        {TEXT("SplatCount"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatsCount"))},
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
  FNDIGaussianSplatProxy& DIProxy = Context.GetProxy<FNDIGaussianSplatProxy>();

  if (Params) {
    UCesiumGaussianSplatSubsystem* SplatSystem = this->GetSubsystem();

    DIProxy.UploadToGPU(SplatSystem);

    Params->SplatsCount =
        IsValid(SplatSystem) ? SplatSystem->GetNumSplats() : 0;
    Params->SplatIndices =
        FNiagaraRenderer::GetSrvOrDefaultUInt(DIProxy.SplatIndicesBuffer.SRV);
    Params->TileTransforms = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.TileTransformsBuffer.SRV);
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
