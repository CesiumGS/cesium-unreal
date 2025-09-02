// Fill out your copyright notice in the Description page of Project Settings.

#include "CesiumGaussianSplatDataInterface.h"

#include "CesiumGaussianSplatSubsystem.h"
#include "CesiumGltfGaussianSplatComponent.h"
#include "Containers/Map.h"
#include "NiagaraCompileHashVisitor.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterface.h"
#include "NiagaraRenderer.h"
#include "NiagaraShaderParametersBuilder.h"
#include "RHICommandList.h"
#include <HLSLTypeAliases.h>
#include <NiagaraCommon.h>
#include <NiagaraDataInterfaceSpline.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

const FString GetPositionFunctionName = TEXT("GetSplat_Position");
const FString GetScaleFunctionName = TEXT("GetSplat_Scale");
const FString GetOrientationFunctionName = TEXT("GetSplat_Orientation");
const FString GetTransformTranslationFunctionName =
    TEXT("GetSplat_Transform_Translation");
const FString GetTransformRotationFunctionName =
    TEXT("GetSplat_Transform_Rotation");
const FString GetCoeff0FunctionName = TEXT("GetSplat_Coeff0");
const FString GetOpacityFunctionName = TEXT("GetSplat_Opacity");
const FString GetSHContributionFunctionName = TEXT("GetSplat_SHContribution");
const FString GetNumSplatsFunctionName = TEXT("GetNumSplats");

void UploadSplatMatrices(
    FRHICommandListImmediate& RHICmdList,
    TArray<UCesiumGltfGaussianSplatComponent*>& Components,
    FReadBuffer& Buffer) {
  if (Buffer.NumBytes > 0) {
    Buffer.Release();
  }

  const int32 Stride = 7;

  Buffer.Initialize(
      RHICmdList,
      TEXT("FNDIGaussianSplatProxy_SplatMatricesBuffer"),
      sizeof(FVector4f),
      Components.Num() * Stride,
      EPixelFormat::PF_A32B32G32R32F,
      BUF_Static);

  float* BufferData = static_cast<float*>(RHICmdList.LockBuffer(
      Buffer.Buffer,
      0,
      Components.Num() * sizeof(FVector4f) * Stride,
      EResourceLockMode::RLM_WriteOnly));
  for (int32 i = 0; i < Components.Num(); i++) {
    glm::mat4x4 mat = Components[i]->GetMatrix();
    const int32 Offset = i * (sizeof(FVector4f) / sizeof(float)) * Stride;
    BufferData[Offset] = (float)mat[0].x;
    BufferData[Offset + 1] = (float)mat[0].y;
    BufferData[Offset + 2] = (float)mat[0].z;
    BufferData[Offset + 3] = (float)mat[0].w;
    BufferData[Offset + 4] = (float)mat[1].x;
    BufferData[Offset + 5] = (float)mat[1].y;
    BufferData[Offset + 6] = (float)mat[1].z;
    BufferData[Offset + 7] = (float)mat[1].w;
    BufferData[Offset + 8] = (float)mat[2].x;
    BufferData[Offset + 9] = (float)mat[2].y;
    BufferData[Offset + 10] = (float)mat[2].z;
    BufferData[Offset + 11] = (float)mat[2].w;
    BufferData[Offset + 12] = (float)mat[3].x;
    BufferData[Offset + 13] = (float)mat[3].y;
    BufferData[Offset + 14] = (float)mat[3].z;
    BufferData[Offset + 15] = (float)mat[3].w;
    const FTransform& ComponentToWorld = Components[i]->GetComponentToWorld();
    FVector Location = ComponentToWorld.GetLocation();
    BufferData[Offset + 16] = (float)Location.X;
    BufferData[Offset + 17] = (float)Location.Y;
    BufferData[Offset + 18] = (float)Location.Z;
    BufferData[Offset + 19] = (float)1.0;
    FVector Scale = ComponentToWorld.GetScale3D();
    BufferData[Offset + 20] = (float)Scale.X;
    BufferData[Offset + 21] = (float)Scale.Y;
    BufferData[Offset + 22] = (float)Scale.Z;
    BufferData[Offset + 23] = (float)1.0;
    FQuat Rotation = ComponentToWorld.GetRotation();
    BufferData[Offset + 24] = (float)Rotation.X;
    BufferData[Offset + 25] = (float)Rotation.Y;
    BufferData[Offset + 26] = (float)Rotation.Z;
    BufferData[Offset + 27] = (float)Rotation.W;
  }

  RHICmdList.UnlockBuffer(Buffer.Buffer);
}

void FNDIGaussianSplatProxy::UploadToGPU(UWorld* World) {
  if (this->Owner == nullptr || !IsValid(World)) {
    return;
  }

  UCesiumGaussianSplatSubsystem* SplatSystem =
      World->GetSubsystem<UCesiumGaussianSplatSubsystem>();

  if (!IsValid(SplatSystem)) {
    return;
  }

  if (this->bMatricesNeedUpdate) {
    this->bMatricesNeedUpdate = false;

    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatMatrices)(
        [this, SplatSystem](FRHICommandListImmediate& RHICmdList) {
          UploadSplatMatrices(
              RHICmdList,
              SplatSystem->SplatComponents,
              this->SplatMatricesBuffer);
        });
  }

  if (!this->bNeedsUpdate) {
    return;
  }

  this->bNeedsUpdate = false;

  const int32 NumSplats = SplatSystem->GetNumSplats();

  /*TArray<int> SplatCoeffCounts;
  for (int32 i = 0; i < this->Owner->SplatSystem->SplatActors.Num(); i++) {
    const FGaussianSplatData& SplatData =
        this->Owner->SplatSystem->SplatActors[i]->SplatData;
    int SplatCount = SplatData.HarmonicCoefficients.Num() - 1;
    SplatCoeffCounts.Emplace(SplatCount);
  }*/

  ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatBuffers)(
      [this, SplatSystem, NumSplats](FRHICommandListImmediate& RHICmdList) {
        int32 ExpectedBytes = 0;
        for (UCesiumGltfGaussianSplatComponent* Component :
             SplatSystem->SplatComponents) {
          ExpectedBytes += Component->Data.Num() * sizeof(float);
        }

        if (this->DataBuffer.NumBytes != ExpectedBytes) {
          if (this->DataBuffer.NumBytes > 0) {
            this->DataBuffer.Release();
            this->SplatOffsetsBuffer.Release();
            this->SplatIndicesBuffer.Release();
          }

          if (ExpectedBytes > 0) {
            this->DataBuffer.Initialize(
                RHICmdList,
                TEXT("FNDIGaussianSplatProxy_Data"),
                sizeof(FVector4f),
                ExpectedBytes / 4,
                EPixelFormat::PF_A32B32G32R32F,
                BUF_Static);
            this->SplatIndicesBuffer.Initialize(
                RHICmdList,
                TEXT("FNDIGaussianSplatProxy_SplatIndicesBuffer"),
                sizeof(int32),
                NumSplats,
                EPixelFormat::PF_R32_UINT,
                BUF_Static);
            this->SplatOffsetsBuffer.Initialize(
                RHICmdList,
                TEXT("FNDIGaussianSplatProxy_SplatOffsetsBuffer"),
                sizeof(int32),
                SplatSystem->SplatComponents.Num() * 3,
                EPixelFormat::PF_R32_UINT,
                BUF_Static);
          }
        }

        if (ExpectedBytes > 0) {
          FScopeLock ScopeLock(&this->BufferLock);
          {
            float* DataBuffer = static_cast<float*>(RHICmdList.LockBuffer(
                this->DataBuffer.Buffer,
                0,
                ExpectedBytes,
                EResourceLockMode::RLM_WriteOnly));
            int32* IndexBuffer = static_cast<int32*>(RHICmdList.LockBuffer(
                this->SplatIndicesBuffer.Buffer,
                0,
                NumSplats * sizeof(int32),
                EResourceLockMode::RLM_WriteOnly));
            int32* OffsetsBuffer = static_cast<int32*>(RHICmdList.LockBuffer(
                this->SplatOffsetsBuffer.Buffer,
                0,
                SplatSystem->SplatComponents.Num() * sizeof(int32) * 4,
                EResourceLockMode::RLM_WriteOnly));

            int32 Offset = 0;
            int32 Unoffset = 0;
            for (int32 i = 0; i < SplatSystem->SplatComponents.Num(); i++) {
              UCesiumGltfGaussianSplatComponent* Component =
                  SplatSystem->SplatComponents[i];
              FPlatformMemory::Memcpy(
                  reinterpret_cast<void*>(DataBuffer + Offset),
                  Component->Data.GetData(),
                  Component->Data.Num() * sizeof(float));
              for (int32 j = 0; j < Component->NumSplats; j++) {
                IndexBuffer[Offset + j] = i;
              }

              // OffsetsBuffer contains:
              // - 0: Number of entries into Data to offset by before the first
              // data from this splat component.
              // - 1: Number to subtract from the particle index to get the
              // index into this splat component's data.
              // - 2: Stride between elements of this splat component.
              // - 3: Number of spherical harmonic coefficients.
              OffsetsBuffer[i * 4] = Offset / 4;
              OffsetsBuffer[i * 4 + 1] = Unoffset;
              OffsetsBuffer[i * 4 + 2] = Component->DataStride;
              OffsetsBuffer[i * 4 + 3] = Component->NumCoefficients;

              Offset += Component->Data.Num();
              Unoffset += Component->NumSplats;
            }
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
      TEXT("Buffer<int> %s%s;\n"),
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
      TEXT("_Scales"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_Orientations"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SHZeroCoeffsAndOpacity"));
  OutHLSL.Appendf(
      TEXT("Buffer<int> %s%s;\n"),
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

  static const TCHAR* MatrixMathFormatBounds = TEXT(R"(
		int SplatIndex = {IndicesBuffer}[Index];
		float4 c0 = {MatrixBuffer}[SplatIndex * 7];
		float4 c1 = {MatrixBuffer}[SplatIndex * 7 + 1];
		float4 c2 = {MatrixBuffer}[SplatIndex * 7 + 2];
		float4 c3 = {MatrixBuffer}[SplatIndex * 7 + 3];
		float4x4 SplatMatrix = float4x4(
			c0.x, c1.x, c2.x, c3.x,
			c0.y, c1.y, c2.y, c3.y,
			c0.z, c1.z, c2.z, c3.z,
			c0.w, c1.w, c2.w, c3.w
		);
	)");

  const TMap<FString, FStringFormatArg> MatrixMathArgsBounds = {
      {TEXT("IndicesBuffer"),
       FStringFormatArg(
           ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
      {TEXT("MatrixBuffer"),
       FStringFormatArg(
           ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatMatrices"))}};

  if (FunctionInfo.DefinitionName == *GetPositionFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float3 OutPosition)
		{
			{MatrixMath}
			OutPosition = mul(SplatMatrix, float4({Buffer}[Index].xyz, 1.0)).xyz;
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("MatrixMath"),
         FStringFormatArg(
             FString::Format(MatrixMathFormatBounds, MatrixMathArgsBounds))},
        {TEXT("Buffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Positions"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetScaleFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float3 OutScale)
		{
			int SplatIndex = {IndicesBuffer}[Index];
			float4 SScale = {MatrixBuffer}[SplatIndex * 7 + 5];
			OutScale = SScale * {Buffer}[Index].xyz;
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("Buffer"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Scales"))},
        {TEXT("IndicesBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
        {TEXT("MatrixBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatMatrices"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetOrientationFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float4 OutOrientation)
		{
			int SplatIndex = {IndicesBuffer}[Index];
			float4 q2 = {Buffer}[Index];
			float4 q1 = {MatrixBuffer}[SplatIndex * 7 + 6];
			// Multiply the two quaternions
			OutOrientation = q2;/*float4(
				q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
				q1.w * q2.w - dot(q1.xyz, q2.xyz)
			);*/
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("Buffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Orientations"))},
        {TEXT("IndicesBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
        {TEXT("MatrixBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatMatrices"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (
      FunctionInfo.DefinitionName == *GetTransformTranslationFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float4 OutTranslation)
		{
			int SplatIndex = {IndicesBuffer}[Index];
			OutTranslation = {MatrixBuffer}[SplatIndex * 7 + 4];
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("IndicesBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
        {TEXT("MatrixBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatMatrices"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetTransformRotationFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float4 OutRotation)
		{
			int SplatIndex = {IndicesBuffer}[Index];
			OutRotation = {MatrixBuffer}[SplatIndex * 7 + 6];
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("IndicesBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
        {TEXT("MatrixBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatMatrices"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetCoeff0FunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float3 OutCoeff0)
		{
			OutCoeff0 = {Buffer}[Index].xyz;
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("Buffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol +
             TEXT("_SHZeroCoeffsAndOpacity"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetOpacityFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float OutOpacity)
		{
			OutOpacity = {Buffer}[Index].w;
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("Buffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol +
             TEXT("_SHZeroCoeffsAndOpacity"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetSHContributionFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(float3 WorldViewDir, int Index, out float3 OutColor)
		{
			const float SH_C1   = 0.4886025119029199f;
			const float SH_C2[] = {1.0925484, -1.0925484, 0.3153916, -1.0925484, 0.5462742};
			const float SH_C3[] = {-0.5900435899266435f, 2.890611442640554f, -0.4570457994644658f, 0.3731763325901154f,
														-0.4570457994644658f, 1.445305721320277f, -0.5900435899266435f};

			int SplatIndex = {SplatIndices}[Index];
			int SHCount = {SHDegrees}[SplatIndex * 3];
			int SHOffset = {SHDegrees}[SplatIndex * 3 + 1] + (Index - {SHDegrees}[SplatIndex * 3 + 2]) * SHCount;

			const float x = WorldViewDir.x;
			const float y = WorldViewDir.y;
			const float z = WorldViewDir.z;
			const float xx = x * x;
			const float yy = y * y;
			const float zz = z * z;
			const float xy = x * y;
			const float yz = y * z;
			const float xz = x * z;
			const float xyz = x * y * z;

			float3 Color = float3(0.0, 0.0, 0.0);
			if(SHCount >= 3) {
				float3 shd1_0 = {Buffer}[SHOffset].xyz;
				float3 shd1_1 = {Buffer}[SHOffset + 1].xyz;
				float3 shd1_2 = {Buffer}[SHOffset + 2].xyz;
				Color += SH_C1 * (-shd1_0 * y + shd1_1 * z - shd1_2 * x);
			}

			if(SHCount >= 8) {
				float3 shd2_0 = {Buffer}[SHOffset + 3];
				float3 shd2_1 = {Buffer}[SHOffset + 4];
				float3 shd2_2 = {Buffer}[SHOffset + 5];
				float3 shd2_3 = {Buffer}[SHOffset + 6];
				float3 shd2_4 = {Buffer}[SHOffset + 7];
				Color += (SH_C2[0] * xy) * shd2_0 + (SH_C2[1] * yz) * shd2_1 + (SH_C2[2] * (2.0 * zz - xx - yy)) * shd2_2
					   + (SH_C2[3] * xz) * shd2_3 + (SH_C2[4] * (xx - yy)) * shd2_4;
			}

			if(SHCount >= 15) {
				float3 shd3_0 = {Buffer}[SHOffset + 8];
				float3 shd3_1 = {Buffer}[SHOffset + 9];
				float3 shd3_2 = {Buffer}[SHOffset + 10];
				float3 shd3_3 = {Buffer}[SHOffset + 11];
				float3 shd3_4 = {Buffer}[SHOffset + 12];
				float3 shd3_5 = {Buffer}[SHOffset + 13];
				float3 shd3_6 = {Buffer}[SHOffset + 14];

				Color += SH_C3[0] * shd3_0 * (3.0 * xx - yy) * y + SH_C3[1] * shd3_1 * xyz
									 + SH_C3[2] * shd3_2 * (4.0 * zz - xx - yy) * y
									 + SH_C3[3] * shd3_3 * z * (2.0 * zz - 3.0 * xx - 3.0 * yy)
									 + SH_C3[4] * shd3_4 * x * (4.0 * zz - xx - yy)
									 + SH_C3[5] * shd3_5 * (xx - yy) * z + SH_C3[6] * shd3_6 * x * (xx - 3.0 * yy);
			}

			OutColor = Color;
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("Buffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SHNonZeroCoeffs"))},
        {TEXT("SHDegrees"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatSHDegrees"))},
        {TEXT("SplatIndices"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
        {TEXT("PosBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Positions"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetNumSplatsFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out int OutNumSplats)
		{
			OutNumSplats = {SplatCount};
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("SplatCount"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatsCount"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
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
  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetPositionFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition::GetVec3Def(),
        TEXT("Position")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetScaleFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Scale")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetOrientationFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition::GetVec4Def(),
        TEXT("Orientation")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetTransformRotationFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition::GetVec4Def(),
        TEXT("Rotation")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetTransformTranslationFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition::GetVec4Def(),
        TEXT("Translation")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetCoeff0FunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Coeff0")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetOpacityFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition::GetFloatDef(),
        TEXT("Opacity")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetSHContributionFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition::GetVec3Def(),
        TEXT("WorldViewDir")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Color")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }

  {
    FNiagaraFunctionSignature Sig;
    Sig.Name = *GetNumSplatsFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Outputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition::GetIntDef(),
        TEXT("NumSplats")));
    Sig.bMemberFunction = true;
    Sig.bRequiresContext = false;
    OutFunctions.Add(Sig);
  }
}

bool UCesiumGaussianSplatDataInterface::CopyToInternal(
    UNiagaraDataInterface* Destination) const {
  if (!UNiagaraDataInterface::CopyToInternal(Destination)) {
    return false;
  }

  UCesiumGaussianSplatDataInterface* CastedDestination =
      Cast<UCesiumGaussianSplatDataInterface>(Destination);
  if (CastedDestination) {
    CastedDestination->SplatSystem = this->SplatSystem;
  }

  return true;
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
    DIProxy.UploadToGPU();
    Params->SplatsCount =
        this->SplatSystem ? this->SplatSystem->GetNumSplats() : 0;
    Params->SplatIndices =
        FNiagaraRenderer::GetSrvOrDefaultInt(DIProxy.SplatIndicesBuffer.SRV);
    Params->SplatMatrices =
        FNiagaraRenderer::GetSrvOrDefaultInt(DIProxy.SplatMatricesBuffer.SRV);
    Params->Positions =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.PositionsBuffer.SRV);
    Params->Scales =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.ScalesBuffer.SRV);
    Params->Orientations =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.OrientationsBuffer.SRV);
    Params->SHZeroCoeffsAndOpacity = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.SHZeroCoeffsAndOpacity.SRV);
    Params->SHNonZeroCoeffs = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.SHNonZeroCoeffsBuffer.SRV);
    Params->SplatSHDegrees =
        FNiagaraRenderer::GetSrvOrDefaultInt(DIProxy.SplatSHDegreesBuffer.SRV);
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

bool UCesiumGaussianSplatDataInterface::Equals(
    const UNiagaraDataInterface* Other) const {
  bool bIsEqual = UNiagaraDataInterface::Equals(Other);
  const UCesiumGaussianSplatDataInterface* OtherSplatInterface =
      Cast<const UCesiumGaussianSplatDataInterface>(Other);
  return bIsEqual && OtherSplatInterface &&
         OtherSplatInterface->SplatSystem == this->SplatSystem;
}

bool UCesiumGaussianSplatDataInterface::CanExecuteOnTarget(
    ENiagaraSimTarget target) const {
  return true;
}

#if WITH_EDITOR
void UCesiumGaussianSplatDataInterface::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  UNiagaraDataInterface::PostEditChangeProperty(PropertyChangedEvent);

  if (!HasAnyFlags(RF_ClassDefaultObject)) {
    if (PropertyChangedEvent.GetMemberPropertyName() ==
        GET_MEMBER_NAME_CHECKED(
            UCesiumGaussianSplatDataInterface,
            SplatSystem)) {
      FNDIGaussianSplatProxy* ThisProxy =
          this->GetProxyAs<FNDIGaussianSplatProxy>();
      ThisProxy->bNeedsUpdate = true;
    }
  }
}
#endif
