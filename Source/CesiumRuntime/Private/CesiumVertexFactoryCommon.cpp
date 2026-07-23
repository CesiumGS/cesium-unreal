// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVertexFactoryCommon.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialDomain.h"
#include "RenderCommandFence.h"
#include "Runtime/Launch/Resources/Version.h"

void FCesiumQuadIndexBuffer::InitRHI(FRHICommandListBase& RHICmdList) {
  if (!this->_manualVertexFetchSupported) {
    return;
  }

  // This must be called from Rendering thread
  check(IsInRenderingThread());

  const uint32 NumIndices = this->_quadCount * 6;
  const uint32 Size = NumIndices * sizeof(uint32);

  FRHIBufferCreateDesc CreateDesc(
      TEXT("FCesiumQuadIndexBuffer"),
      Size,
      sizeof(uint32),
      BUF_Static | BUF_IndexBuffer);
  CreateDesc.SetInitialState(ERHIAccess::VertexOrIndexBuffer);

  IndexBufferRHI = RHICmdList.CreateBuffer(CreateDesc);

  uint32* pData =
      (uint32*)RHICmdList.LockBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);

  for (uint32 index = 0, bufferIndex = 0; bufferIndex < NumIndices;
       index += 4) {
    // Generate six indices per quad.
    pData[bufferIndex++] = index;
    pData[bufferIndex++] = index + 1;
    pData[bufferIndex++] = index + 2;
    pData[bufferIndex++] = index;
    pData[bufferIndex++] = index + 2;
    pData[bufferIndex++] = index + 3;
  }

  RHICmdList.UnlockBuffer(IndexBufferRHI);
}

void FCesiumDummyVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList) {
  FRHIBufferCreateDesc CreateDesc(
      TEXT("FCesiumDummyVertexBuffer"),
      sizeof(FVector3f) * 4,
      0,
      BUF_Static | BUF_VertexBuffer);
  CreateDesc.SetInitialState(ERHIAccess::VertexOrIndexBuffer);
  VertexBufferRHI = RHICmdList.CreateBuffer(CreateDesc);

  FVector3f* pDummyContents = (FVector3f*)RHICmdList.LockBuffer(
      VertexBufferRHI,
      0,
      sizeof(FVector3f) * 4,
      RLM_WriteOnly);
  pDummyContents[0] = FVector3f(0.0f, 0.0f, 0.0f);
  pDummyContents[1] = FVector3f(1.0f, 0.0f, 0.0f);
  pDummyContents[2] = FVector3f(0.0f, 1.0f, 0.0f);
  pDummyContents[3] = FVector3f(1.0f, 1.0f, 0.0f);

  RHICmdList.UnlockBuffer(VertexBufferRHI);
}
