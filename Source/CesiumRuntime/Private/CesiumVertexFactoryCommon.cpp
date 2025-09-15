// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVertexFactoryCommon.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialDomain.h"
#include "RenderCommandFence.h"
#include "Runtime/Launch/Resources/Version.h"

void FCesiumQuadIndexBuffer::InitRHI(FRHICommandListBase& RHICmdList) {
   if (!bManualVertexFetchSupported) {
     return;
   }

  // This must be called from Rendering thread
  check(IsInRenderingThread());

  FRHIResourceCreateInfo CreateInfo(TEXT("FCesiumPolylineIndexBuffer"));

  const uint32 NumIndices = QuadCount * 6;
  const uint32 Size = NumIndices * sizeof(uint32);

  IndexBufferRHI = RHICmdList.CreateBuffer(
      Size,
      BUF_Static | BUF_IndexBuffer,
      sizeof(uint32),
      ERHIAccess::VertexOrIndexBuffer,
      CreateInfo);

  uint32* Data =
      (uint32*)RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);

  for (uint32 index = 0, bufferIndex = 0; bufferIndex < NumIndices;
       index += 4) {
    // Generate six indices per quad.
    Data[bufferIndex++] = index;
    Data[bufferIndex++] = index + 1;
    Data[bufferIndex++] = index + 2;
    Data[bufferIndex++] = index;
    Data[bufferIndex++] = index + 2;
    Data[bufferIndex++] = index + 3;
  }

  RHICmdList.UnlockBuffer(IndexBufferRHI);
}

void FCesiumDummyVertexBuffer::InitRHI(
    FRHICommandListBase& RHICmdList) {
  FRHIResourceCreateInfo CreateInfo(TEXT("FCesiumDummyVertexBuffer"));
  VertexBufferRHI = RHICmdList.CreateBuffer(
      sizeof(FVector3f) * 4,
      BUF_Static | BUF_VertexBuffer,
      0,
      ERHIAccess::VertexOrIndexBuffer,
      CreateInfo);
  FVector3f* DummyContents = (FVector3f*)RHICmdList.LockBuffer(
      VertexBufferRHI,
      0,
      sizeof(FVector3f) * 4,
      RLM_WriteOnly);
  DummyContents[0] = FVector3f(0.0f, 0.0f, 0.0f);
  DummyContents[1] = FVector3f(1.0f, 0.0f, 0.0f);
  DummyContents[2] = FVector3f(0.0f, 1.0f, 0.0f);
  DummyContents[3] = FVector3f(1.0f, 1.0f, 0.0f);
  RHICmdList.UnlockBuffer(VertexBufferRHI);
}
