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

void updateVisibility(
    FRHICommandListImmediate& RHICmdList,
    TArray<UCesiumGltfGaussianSplatComponent*>& Components,
    FReadBuffer& Buffer) {
  releaseIfNonEmpty(Buffer);

  if (Components.IsEmpty()) {
    // Will crash if we try to allocate a buffer of size 0
    return;
  }

  Buffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_VisibilityBuffer"),
      sizeof(uint32),
      Components.Num(),
      EPixelFormat::PF_R32_UINT,
      BUF_Static);

  uint32* pVisibilityData = static_cast<uint32*>(RHICmdList.LockBuffer(
      Buffer.Buffer,
      0,
      Components.Num() * sizeof(uint32),
      EResourceLockMode::RLM_WriteOnly));

  for (int32 c = 0; c < Components.Num(); c++) {
    UCesiumGltfGaussianSplatComponent* pComponent = Components[c];
    check(pComponent);

    pVisibilityData[c] = pComponent->IsVisible() ? 1u : 0u;
  }

  RHICmdList.UnlockBuffer(Buffer.Buffer);
}

void updateWorldSpaceParameters(
    FRHICommandListImmediate& RHICmdList,
    TArray<UCesiumGltfGaussianSplatComponent*>& Components,
    FReadBuffer& TileRotationBuffer,
    FReadBuffer& CovarianceMatrixBuffer,
    FReadBuffer& PositionsBuffer,
    int32 NumSplats) {
  releaseIfNonEmpty(TileRotationBuffer);
  releaseIfNonEmpty(CovarianceMatrixBuffer);
  releaseIfNonEmpty(PositionsBuffer);

  if (Components.IsEmpty()) {
    // Will crash if we try to allocate a buffer of size 0
    return;
  }

  const int32 CovarianceMatrixStride = 3 * sizeof(FVector4f) / sizeof(float);

  TileRotationBuffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_SplatMatricesBuffer"),
      sizeof(FVector4f),
      Components.Num(),
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  CovarianceMatrixBuffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_CovarianceMatricesBuffer"),
      sizeof(FVector4f),
      NumSplats * 3,
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  PositionsBuffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_Positions"),
      sizeof(FVector4f),
      NumSplats,
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  FVector4f* pTileRotationData =
      reinterpret_cast<FVector4f*>(RHICmdList.LockBuffer(
          TileRotationBuffer.Buffer,
          0,
          Components.Num() * sizeof(FVector4f),
          EResourceLockMode::RLM_WriteOnly));

  float* pCovarianceData = static_cast<float*>(RHICmdList.LockBuffer(
      CovarianceMatrixBuffer.Buffer,
      0,
      NumSplats * 3 * sizeof(FVector4f),
      EResourceLockMode::RLM_WriteOnly));

  float* pPositionData = static_cast<float*>(RHICmdList.LockBuffer(
      PositionsBuffer.Buffer,
      0,
      NumSplats * sizeof(FVector4f),
      EResourceLockMode::RLM_WriteOnly));

  int32 covarianceOffset = 0;
  int32 positionOffset = 0;

  for (int32 c = 0; c < Components.Num(); c++) {
    UCesiumGltfGaussianSplatComponent* pComponent = Components[c];
    check(pComponent);

    const FTransform& ComponentToWorld = pComponent->GetComponentToWorld();
    FQuat tileRotation = ComponentToWorld.GetRotation();
    tileRotation.Normalize();

    pTileRotationData[c] = FVector4f(
        static_cast<float>(tileRotation.X),
        static_cast<float>(tileRotation.Y),
        static_cast<float>(tileRotation.Z),
        static_cast<float>(tileRotation.W));

    glm::dmat4 tileMatrix = pComponent->GetMatrix();
    FVector tileScale = ComponentToWorld.GetScale3D();

    // Compute the world-space covariance of each splat from its scale and
    // rotation. Doing this on the double-precision on the CPU will mitigate
    // precision issues in the shader.
    const int32 splatCount = pComponent->Rotations.Num() / 4;
    for (int32 i = 0; i < splatCount; i++) {
      glm::dvec3 scale = VecMath::createVector3D(tileScale) *
                         glm::dvec3(
                             pComponent->Scales[4 * i + 0],
                             pComponent->Scales[4 * i + 1],
                             pComponent->Scales[4 * i + 2]);

      glm::dquat rotation(
          double(pComponent->Rotations[4 * i + 3]),
          double(pComponent->Rotations[4 * i + 0]),
          double(pComponent->Rotations[4 * i + 1]),
          double(pComponent->Rotations[4 * i + 2]));

      glm::dmat3 S =
          glm::dmat3(glm::scale(glm::identity<glm::dmat4x4>(), scale));
      glm::dmat3 R = glm::mat3_cast(rotation);

      // See the KHR_gaussian_splatting spec for the covariance matrix math.
      // https://github.com/CesiumGS/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting#3d-gaussian-representation
      glm::dmat3 M = S * R;
      glm::dmat3 sigma = transpose(M) * M;

      // Transpose to row order, which is used by the HLSL shader.
      pCovarianceData[covarianceOffset + 0] = static_cast<float>(sigma[0].x);
      pCovarianceData[covarianceOffset + 1] = static_cast<float>(sigma[1].x);
      pCovarianceData[covarianceOffset + 2] = static_cast<float>(sigma[2].x);
      pCovarianceData[covarianceOffset + 3] = 0.0f;
      pCovarianceData[covarianceOffset + 4] = static_cast<float>(sigma[0].y);
      pCovarianceData[covarianceOffset + 5] = static_cast<float>(sigma[1].y);
      pCovarianceData[covarianceOffset + 6] = static_cast<float>(sigma[2].y);
      pCovarianceData[covarianceOffset + 7] = 0.0f;
      pCovarianceData[covarianceOffset + 8] = static_cast<float>(sigma[0].z);
      pCovarianceData[covarianceOffset + 9] = static_cast<float>(sigma[1].z);
      pCovarianceData[covarianceOffset + 10] = static_cast<float>(sigma[2].z);
      pCovarianceData[covarianceOffset + 11] = 0.0f;

      covarianceOffset += CovarianceMatrixStride;

      glm::dvec4 position(
          pComponent->Positions[4 * i + 0],
          pComponent->Positions[4 * i + 1],
          pComponent->Positions[4 * i + 2],
          1.0);

      glm::dvec4 worldPosition = tileMatrix * position;

      pPositionData[positionOffset + 0] = static_cast<float>(worldPosition.x);
      pPositionData[positionOffset + 1] = static_cast<float>(worldPosition.y);
      pPositionData[positionOffset + 2] = static_cast<float>(worldPosition.z);
      pPositionData[positionOffset + 3] = 1.0f;

      positionOffset += 4;
    }
  }

  RHICmdList.UnlockBuffer(TileRotationBuffer.Buffer);
  RHICmdList.UnlockBuffer(CovarianceMatrixBuffer.Buffer);
  RHICmdList.UnlockBuffer(PositionsBuffer.Buffer);
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

  if (this->bVisibilityNeedsUpdate) {
    this->bVisibilityNeedsUpdate = false;

    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatMatrices)
    ([this, SplatSystem](FRHICommandListImmediate& RHICmdList) {
      if (!IsValid(SplatSystem))
        return;

      FScopeLock ScopeLock(&this->BufferLock);
      updateVisibility(
          RHICmdList,
          SplatSystem->SplatComponents,
          this->TileVisibilityBuffer);
    });
  }

  if (this->bMatricesNeedUpdate) {
    this->bMatricesNeedUpdate = false;

    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatMatrices)
    ([this, SplatSystem](FRHICommandListImmediate& RHICmdList) {
      if (!IsValid(SplatSystem))
        return;

      FScopeLock ScopeLock(&this->BufferLock);
      updateWorldSpaceParameters(
          RHICmdList,
          SplatSystem->SplatComponents,
          this->TileRotationsBuffer,
          this->CovarianceMatrixBuffer,
          this->PositionsBuffer,
          SplatSystem->GetNumSplats());
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

    const int32 ExpectedColorBytes = NumSplats * 4 * sizeof(float);
    if (this->ColorsBuffer.NumBytes != ExpectedColorBytes) {
      releaseIfNonEmpty(this->TileIndicesBuffer);
      releaseIfNonEmpty(this->ColorsBuffer);
      releaseIfNonEmpty(this->SHNonZeroCoeffsBuffer);
      releaseIfNonEmpty(this->SplatSHDegreesBuffer);

      if (SplatSystem->SplatComponents.IsEmpty()) {
        return;
      }

      if (ExpectedColorBytes > 0) {
        this->TileIndicesBuffer.Initialize(
            RHICmdList,
            TEXT("FNDIGaussianSplatProxy_TileIndicesBuffer"),
            sizeof(uint32),
            NumSplats,
            EPixelFormat::PF_R32_UINT,
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

        uint32* pIndexBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
            this->TileIndicesBuffer.Buffer,
            0,
            NumSplats * sizeof(uint32),
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
        uint32* SHDegreesBuffer = static_cast<uint32*>(RHICmdList.LockBuffer(
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
            pIndexBuffer[SplatCountWritten + j] = static_cast<uint32>(i);
          }

          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(ColorsBuffer + SplatCountWritten * 4),
              Component->Colors.GetData(),
              Component->Colors.Num() * sizeof(float));
          if (TotalCoeffsCount > 0) {
            FPlatformMemory::Memcpy(
                reinterpret_cast<void*>(
                    SHNonZeroCoeffsBuffer + CoeffCountWritten * 4),
                Component->SphericalHarmonics.GetData(),
                Component->SphericalHarmonics.Num() * sizeof(float));
          }

          SHDegreesBuffer[i * 3] =
              static_cast<uint32>(Component->NumCoefficients);
          SHDegreesBuffer[i * 3 + 1] = static_cast<uint32>(CoeffCountWritten);
          SHDegreesBuffer[i * 3 + 2] = static_cast<uint32>(SplatCountWritten);

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
      TEXT("Buffer<uint> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_TileVisibility"));
  OutHLSL.Appendf(
      TEXT("Buffer<uint> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_TileIndices"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_TileRotations"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_Positions"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_CovarianceMatrices"));
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
        {TEXT("TileVisibilityBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileVisibility"))},
        {TEXT("TileRotationBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileRotations"))},
        {TEXT("TileIndicesBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_TileIndices"))},
        {TEXT("PositionsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Positions"))},
        {TEXT("ColorsBuffer"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Colors"))},
        {TEXT("CovarianceMatrixBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_CovarianceMatrices"))},
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

    Params->TileVisibility =
        FNiagaraRenderer::GetSrvOrDefaultUInt(DIProxy.TileVisibilityBuffer.SRV);
    Params->TileIndices =
        FNiagaraRenderer::GetSrvOrDefaultUInt(DIProxy.TileIndicesBuffer.SRV);
    Params->TileRotations = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.TileRotationsBuffer.SRV);
    Params->CovarianceMatrices = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.CovarianceMatrixBuffer.SRV);
    Params->Positions =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.PositionsBuffer.SRV);
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

void UCesiumGaussianSplatDataInterface::RefreshVisibility() {
  this->GetProxyAs<FNDIGaussianSplatProxy>()->bVisibilityNeedsUpdate = true;
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
