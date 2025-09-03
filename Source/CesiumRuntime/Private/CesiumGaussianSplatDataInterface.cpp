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
const FString GetColorFunctionName = TEXT("GetSplat_Color");
const FString GetSHContributionFunctionName = TEXT("GetSplat_SHContribution");
const FString GetNumSplatsFunctionName = TEXT("GetNumSplats");

namespace {
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
} // namespace

FNDIGaussianSplatProxy::FNDIGaussianSplatProxy(
    UCesiumGaussianSplatDataInterface* InOwner)
    : Owner(InOwner) {}

void FNDIGaussianSplatProxy::UploadToGPU(
    UCesiumGaussianSplatSubsystem* SplatSystem) {
  if (this->Owner == nullptr || !IsValid(SplatSystem)) {
    return;
  }

  if (this->bMatricesNeedUpdate) {
    this->bMatricesNeedUpdate = false;

    ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatMatrices)
    ([this, SplatSystem](FRHICommandListImmediate& RHICmdList) {
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

  ENQUEUE_RENDER_COMMAND(FUpdateGaussianSplatBuffers)
  ([this, SplatSystem, NumSplats](FRHICommandListImmediate& RHICmdList) {
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
          //
          // This means the data of a given gaussian splat can be found by:
          //   int SplatIndex = _SplatIndices[Index];
          //   int DataIndex = _Offsets[SplatIndex * 4] + _Offsets[i * 4 +
          //   2] * (Index - _Offsets[i * 4 + 1]); _Data[DataIndex];
          OffsetsBuffer[i * 4] = Offset / 4;
          OffsetsBuffer[i * 4 + 1] = Unoffset;
          OffsetsBuffer[i * 4 + 2] = Component->DataStride / 4;
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
      TEXT("Buffer<int> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SplatOffsets"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_SplatMatrices"));
  OutHLSL.Appendf(
      TEXT("Buffer<float4> %s%s;\n"),
      *ParamInfo.DataInterfaceHLSLSymbol,
      TEXT("_Data"));
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

  static const TCHAR* DataIndexFormatBounds = TEXT(R"(
     int SplatIndex = {IndicesBuffer}[Index];
     int DataIndex =
       {OffsetsBuffer}[SplatIndex * 4] +
       {OffsetsBuffer}[SplatIndex * 4 + 2] *
       (Index - {OffsetsBuffer}[SplatIndex * 4 + 1]);
  )");

  const TMap<FString, FStringFormatArg> DataIndexArgsBounds = {
      {TEXT("IndicesBuffer"),
       FStringFormatArg(
           ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
      {TEXT("OffsetsBuffer"),
       FStringFormatArg(
           ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatOffsets"))}};

  static const TCHAR* MatrixMathFormatBounds = TEXT(R"(
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
      {DataIndex}
			{MatrixMath}
			OutPosition = mul(SplatMatrix, float4({Data}[DataIndex].xyz, 1.0)).xyz;
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("MatrixMath"),
         FStringFormatArg(
             FString::Format(MatrixMathFormatBounds, MatrixMathArgsBounds))},
        {TEXT("DataIndex"),
         FStringFormatArg(
             FString::Format(DataIndexFormatBounds, DataIndexArgsBounds))},
        {TEXT("Data"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Data"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetScaleFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float3 OutScale)
		{
      {DataIndex}
			float4 SScale = {MatrixBuffer}[SplatIndex * 7 + 5];
			OutScale = SScale * float3({Data}[DataIndex].w, {Data}[DataIndex + 1].x, {Data}[DataIndex + 1].y);
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("DataIndex"),
         FStringFormatArg(
             FString::Format(DataIndexFormatBounds, DataIndexArgsBounds))},
        {TEXT("Data"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Data"))},
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
      {DataIndex}
			float4 q2 = float4(
        {Data}[DataIndex + 1].z,
        {Data}[DataIndex + 1].w,
        {Data}[DataIndex + 2].x,
        {Data}[DataIndex + 2].y);
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
        {TEXT("DataIndex"),
         FStringFormatArg(
             FString::Format(DataIndexFormatBounds, DataIndexArgsBounds))},
        {TEXT("Data"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Data"))},
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
  } else if (FunctionInfo.DefinitionName == *GetColorFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(int Index, out float4 OutColor)
		{
      {DataIndex}
			OutColor = float4(
        {Data}[DataIndex + 2].z,
        {Data}[DataIndex + 2].w,
        {Data}[DataIndex + 3].x,
        {Data}[DataIndex + 3].y);
		}
		)");

    const TMap<FString, FStringFormatArg> ArgsBounds = {
        {TEXT("FunctionName"), FStringFormatArg(FunctionInfo.InstanceName)},
        {TEXT("DataIndex"),
         FStringFormatArg(
             FString::Format(DataIndexFormatBounds, DataIndexArgsBounds))},
        {TEXT("Data"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Data"))}};

    OutHLSL += FString::Format(FormatBounds, ArgsBounds);
  } else if (FunctionInfo.DefinitionName == *GetSHContributionFunctionName) {
    static const TCHAR* FormatBounds = TEXT(R"(
		void {FunctionName}(float3 WorldViewDir, int Index, out float3 OutColor)
		{
			const float SH_C1   = 0.4886025119029199f;
			const float SH_C2[] = {1.0925484, -1.0925484, 0.3153916, -1.0925484, 0.5462742};
			const float SH_C3[] = {-0.5900435899266435f, 2.890611442640554f, -0.4570457994644658f, 0.3731763325901154f,
														-0.4570457994644658f, 1.445305721320277f, -0.5900435899266435f};

			{DataIndex}
			int SHCount = {OffsetsBuffer}[SplatIndex * 4 + 3];

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
				float3 shd1_0 = float3(
          {Data}[DataIndex + 3].z,
          {Data}[DataIndex + 3].w,
          {Data}[DataIndex + 4].x);
				float3 shd1_1 = float3(
          {Data}[DataIndex + 4].y,
          {Data}[DataIndex + 4].z,
          {Data}[DataIndex + 4].w);
				float3 shd1_2 = float3(
          {Data}[DataIndex + 5].x,
          {Data}[DataIndex + 5].y,
          {Data}[DataIndex + 5].z);
				Color += SH_C1 * (-shd1_0 * y + shd1_1 * z - shd1_2 * x);
			}

			if(SHCount >= 8) {
				float3 shd2_0 = float3(
          {Data}[DataIndex + 5].w,
          {Data}[DataIndex + 6].x,
          {Data}[DataIndex + 6].y);
				float3 shd2_1 = float3(
          {Data}[DataIndex + 6].z,
          {Data}[DataIndex + 6].w,
          {Data}[DataIndex + 7].x);
				float3 shd2_2 = float3(
          {Data}[DataIndex + 7].y,
          {Data}[DataIndex + 7].z,
          {Data}[DataIndex + 7].w);
				float3 shd2_3 = float3(
          {Data}[DataIndex + 8].x,
          {Data}[DataIndex + 8].y,
          {Data}[DataIndex + 8].z);
				float3 shd2_4 = float3(
          {Data}[DataIndex + 8].w,
          {Data}[DataIndex + 9].x,
          {Data}[DataIndex + 9].y);
				Color += (SH_C2[0] * xy) * shd2_0 + (SH_C2[1] * yz) * shd2_1 + (SH_C2[2] * (2.0 * zz - xx - yy)) * shd2_2
					   + (SH_C2[3] * xz) * shd2_3 + (SH_C2[4] * (xx - yy)) * shd2_4;
			}

			if(SHCount >= 15) {
				float3 shd3_0 = float3(
          {Data}[DataIndex + 9].z,
          {Data}[DataIndex + 9].w,
          {Data}[DataIndex + 10].x);
				float3 shd3_1 = float3(
          {Data}[DataIndex + 10].y,
          {Data}[DataIndex + 10].z,
          {Data}[DataIndex + 10].w);
				float3 shd3_2 = float3(
          {Data}[DataIndex + 11].x,
          {Data}[DataIndex + 11].y,
          {Data}[DataIndex + 11].z);
				float3 shd3_3 = float3(
          {Data}[DataIndex + 11].w,
          {Data}[DataIndex + 12].x,
          {Data}[DataIndex + 12].y);
				float3 shd3_4 = float3(
          {Data}[DataIndex + 12].z,
          {Data}[DataIndex + 12].w,
          {Data}[DataIndex + 13].x);
				float3 shd3_5 = float3(
          {Data}[DataIndex + 13].y,
          {Data}[DataIndex + 13].z,
          {Data}[DataIndex + 13].w);
				float3 shd3_6 = float3(
          {Data}[DataIndex + 14].x,
          {Data}[DataIndex + 14].y,
          {Data}[DataIndex + 14].z);

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
        {TEXT("SplatIndices"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatIndices"))},
        {TEXT("OffsetsBuffer"),
         FStringFormatArg(
             ParamInfo.DataInterfaceHLSLSymbol + TEXT("_SplatOffsets"))},
        {TEXT("DataIndex"),
         FStringFormatArg(
             FString::Format(DataIndexFormatBounds, DataIndexArgsBounds))},
        {TEXT("Data"),
         FStringFormatArg(ParamInfo.DataInterfaceHLSLSymbol + TEXT("_Data"))}};

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
    Sig.Name = *GetColorFunctionName;
    Sig.Inputs.Add(FNiagaraVariable(
        FNiagaraTypeDefinition(GetClass()),
        TEXT("GaussianSplatNDI")));
    Sig.Inputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(
        FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Color")));
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
  return UNiagaraDataInterface::CopyToInternal(Destination);
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

  UCesiumGaussianSplatSubsystem* SplatSystem = nullptr;
  UWorld* World = GetWorld();
  if (World) {
    SplatSystem = World->GetSubsystem<UCesiumGaussianSplatSubsystem>();
  }

  if (Params) {
    DIProxy.UploadToGPU(SplatSystem);

    Params->SplatsCount =
        IsValid(SplatSystem) ? SplatSystem->GetNumSplats() : 0;
    Params->SplatIndices =
        FNiagaraRenderer::GetSrvOrDefaultInt(DIProxy.SplatIndicesBuffer.SRV);
    Params->SplatMatrices = FNiagaraRenderer::GetSrvOrDefaultFloat4(
        DIProxy.SplatMatricesBuffer.SRV);
    Params->SplatOffsets =
        FNiagaraRenderer::GetSrvOrDefaultInt(DIProxy.SplatOffsetsBuffer.SRV);
    Params->Data =
        FNiagaraRenderer::GetSrvOrDefaultFloat4(DIProxy.DataBuffer.SRV);
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
  return bIsEqual;
}

bool UCesiumGaussianSplatDataInterface::CanExecuteOnTarget(
    ENiagaraSimTarget target) const {
  return true;
}

#if WITH_EDITOR
void UCesiumGaussianSplatDataInterface::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  UNiagaraDataInterface::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UCesiumGaussianSplatDataInterface::Refresh() {
  this->GetProxyAs<FNDIGaussianSplatProxy>()->bNeedsUpdate = true;
  this->GetProxyAs<FNDIGaussianSplatProxy>()->bMatricesNeedUpdate = true;
}

void UCesiumGaussianSplatDataInterface::RefreshMatrices() {
  this->GetProxyAs<FNDIGaussianSplatProxy>()->bMatricesNeedUpdate = true;
}
