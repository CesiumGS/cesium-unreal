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

void UploadSplatMatrices(
    FRHICommandListImmediate& RHICmdList,
    TArray<UCesiumGltfGaussianSplatComponent*>& Components,
    FReadBuffer& TileMatrixBuffer,
    FReadBuffer& CovarianceMatrixBuffer,
    int32 NumSplats) {
  releaseIfNonEmpty(TileMatrixBuffer);
  releaseIfNonEmpty(CovarianceMatrixBuffer);

  if (Components.IsEmpty()) {
    // Will crash if we try to allocate a buffer of size 0
    return;
  }

  const int32 TileMatrixStride = 7 * sizeof(FVector4f) / sizeof(float);
  const int32 CovarianceMatrixStride = 3 * sizeof(FVector4f) / sizeof(float);

  TileMatrixBuffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_SplatMatricesBuffer"),
      sizeof(FVector4f),
      Components.Num() * TileMatrixStride,
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  CovarianceMatrixBuffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_CovarianceMatricesBuffer"),
      sizeof(FVector4f),
      NumSplats * 3,
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  float* pTileMatrixData = static_cast<float*>(RHICmdList.LockBuffer(
      TileMatrixBuffer.Buffer,
      0,
      Components.Num() * sizeof(FVector4f) * TileMatrixStride,
      EResourceLockMode::RLM_WriteOnly));

  float* pCovarianceData = static_cast<float*>(RHICmdList.LockBuffer(
      CovarianceMatrixBuffer.Buffer,
      0,
      NumSplats * 3 * sizeof(FVector4f),
      EResourceLockMode::RLM_WriteOnly));

  int32 tileMatrixOffset = 0;
  int32 covarianceOffset = 0;

  for (int32 c = 0; c < Components.Num(); c++) {
    UCesiumGltfGaussianSplatComponent* pComponent = Components[c];
    check(pComponent);

    const FTransform& ComponentToWorld = pComponent->GetComponentToWorld();
    FVector tileScale = ComponentToWorld.GetScale3D();
    FQuat tileRotation = ComponentToWorld.GetRotation();

    glm::mat4x4 tileMatrix = pComponent->GetMatrix();
    pTileMatrixData[tileMatrixOffset] = (float)tileMatrix[0].x;
    pTileMatrixData[tileMatrixOffset + 1] = (float)tileMatrix[0].y;
    pTileMatrixData[tileMatrixOffset + 2] = (float)tileMatrix[0].z;
    pTileMatrixData[tileMatrixOffset + 3] = (float)tileMatrix[0].w;
    pTileMatrixData[tileMatrixOffset + 4] = (float)tileMatrix[1].x;
    pTileMatrixData[tileMatrixOffset + 5] = (float)tileMatrix[1].y;
    pTileMatrixData[tileMatrixOffset + 6] = (float)tileMatrix[1].z;
    pTileMatrixData[tileMatrixOffset + 7] = (float)tileMatrix[1].w;
    pTileMatrixData[tileMatrixOffset + 8] = (float)tileMatrix[2].x;
    pTileMatrixData[tileMatrixOffset + 9] = (float)tileMatrix[2].y;
    pTileMatrixData[tileMatrixOffset + 10] = (float)tileMatrix[2].z;
    pTileMatrixData[tileMatrixOffset + 11] = (float)tileMatrix[2].w;
    pTileMatrixData[tileMatrixOffset + 12] = (float)tileMatrix[3].x;
    pTileMatrixData[tileMatrixOffset + 13] = (float)tileMatrix[3].y;
    pTileMatrixData[tileMatrixOffset + 14] = (float)tileMatrix[3].z;
    pTileMatrixData[tileMatrixOffset + 15] = (float)tileMatrix[3].w;

    // Previously these four held location information, but we don't use it
    // (we just use the matrix for that). So, instead, the first component
    // holds visibility and the other three are currently unused.
    pTileMatrixData[tileMatrixOffset + 16] =
        pComponent->IsVisible() ? 1.0f : 0.0f;
    pTileMatrixData[tileMatrixOffset + 17] = 0.0f;
    pTileMatrixData[tileMatrixOffset + 18] = 0.0f;
    pTileMatrixData[tileMatrixOffset + 19] = 0.0f;

    pTileMatrixData[tileMatrixOffset + 20] = (float)tileScale.X;
    pTileMatrixData[tileMatrixOffset + 21] = (float)tileScale.Y;
    pTileMatrixData[tileMatrixOffset + 22] = (float)tileScale.Z;
    pTileMatrixData[tileMatrixOffset + 23] = (float)1.0;

    pTileMatrixData[tileMatrixOffset + 24] = (float)tileRotation.X;
    pTileMatrixData[tileMatrixOffset + 25] = (float)tileRotation.Y;
    pTileMatrixData[tileMatrixOffset + 26] = (float)tileRotation.Z;
    pTileMatrixData[tileMatrixOffset + 27] = (float)tileRotation.W;

    tileMatrixOffset += TileMatrixStride;

    // Compute the world-space covariance of each splat from its scale and
    // rotation. Doing this on the double-precision on the CPU will mitigate
    // precision issues in the shader.
    const int32 splatCount = pComponent->Orientations.Num() / 4;
    for (int32 i = 0; i < splatCount; i++) {
      glm::dvec3 scale = VecMath::createVector3D(tileScale) *
                         glm::dvec3(
                             pComponent->Scales[4 * i + 0],
                             pComponent->Scales[4 * i + 1],
                             pComponent->Scales[4 * i + 2]);

      // TODO: why not tile rotation here?
      glm::dquat rotation(
          double(pComponent->Orientations[4 * i + 3]),
          double(pComponent->Orientations[4 * i + 0]),
          double(pComponent->Orientations[4 * i + 1]),
          double(pComponent->Orientations[4 * i + 2]));

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
    }
  }

  RHICmdList.UnlockBuffer(TileMatrixBuffer.Buffer);
  RHICmdList.UnlockBuffer(CovarianceMatrixBuffer.Buffer);
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
      UploadSplatMatrices(
          RHICmdList,
          SplatSystem->SplatComponents,
          this->SplatMatricesBuffer,
          this->CovarianceMatrixBuffer,
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

    const int32 ExpectedPosBytes = NumSplats * 4 * sizeof(float);
    if (this->PositionsBuffer.NumBytes != ExpectedPosBytes) {
      releaseIfNonEmpty(this->PositionsBuffer);
      releaseIfNonEmpty(this->ScalesBuffer);
      releaseIfNonEmpty(this->OrientationsBuffer);
      releaseIfNonEmpty(this->ColorsBuffer);
      releaseIfNonEmpty(this->SHNonZeroCoeffsBuffer);
      releaseIfNonEmpty(this->SplatSHDegreesBuffer);
      releaseIfNonEmpty(this->SplatIndicesBuffer);

      if (SplatSystem->SplatComponents.IsEmpty()) {
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
            SplatSystem->SplatComponents.Num() * 3,
            EPixelFormat::PF_R32_UINT,
            BUF_Static);

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
            SplatSystem->SplatComponents.Num() * sizeof(uint32) * 3,
            EResourceLockMode::RLM_WriteOnly));

        int32 CoeffCountWritten = 0;
        int32 SplatCountWritten = 0;
        for (int32 i = 0; i < SplatSystem->SplatComponents.Num(); i++) {
          UCesiumGltfGaussianSplatComponent* Component =
              SplatSystem->SplatComponents[i];
          check(Component);

          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(PositionsBuffer + SplatCountWritten * 4),
              Component->Positions.GetData(),
              Component->Positions.Num() * sizeof(float));

          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(ScalesBuffer + SplatCountWritten * 4),
              Component->Scales.GetData(),
              Component->Scales.Num() * sizeof(float));
          FPlatformMemory::Memcpy(
              reinterpret_cast<void*>(
                  OrientationsBuffer + SplatCountWritten * 4),
              Component->Orientations.GetData(),
              Component->Orientations.Num() * sizeof(float));

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
          for (int32 j = 0; j < Component->NumSplats; j++) {
            IndexBuffer[SplatCountWritten + j] = static_cast<uint32>(i);
          }

          SHDegreesBuffer[i * 3] =
              static_cast<uint32>(Component->NumCoefficients);
          SHDegreesBuffer[i * 3 + 1] = static_cast<uint32>(CoeffCountWritten);
          SHDegreesBuffer[i * 3 + 2] = static_cast<uint32>(SplatCountWritten);

          SplatCountWritten += Component->NumSplats;
          CoeffCountWritten +=
              Component->NumSplats * Component->NumCoefficients;
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
      TEXT("_SplatMatrices"));
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
        {TEXT("TileMatrixBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatMatrices"))},
        {TEXT("SHCoeffs"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SHNonZeroCoeffs"))},
        {TEXT("SHDegrees"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatSHDegrees"))},
        {TEXT("SplatCount"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatsCount"))},
        {TEXT("CovarianceMatrixBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_CovarianceMatrices"))},
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
    Params->SplatMatrices = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.SplatMatricesBuffer.SRV);
    Params->CovarianceMatrices = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.CovarianceMatrixBuffer.SRV);
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
