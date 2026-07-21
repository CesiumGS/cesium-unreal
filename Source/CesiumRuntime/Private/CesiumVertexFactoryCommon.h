// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RenderResource.h"
#include "Runtime/Launch/Resources/Version.h"

/**
 * An index buffer that is generated for the specified number of quads
 * (two triangles per quad). This is most helpful for techniques that require
 * screens-space billboarded quads, such as point attenuation and thick polyline
 * rendering.
 */
class FCesiumQuadIndexBuffer : public FIndexBuffer {
public:
  FCesiumQuadIndexBuffer(
      const int32& InQuadCount,
      const bool InManualVertexFetchSupported)
      : QuadCount(InQuadCount),
        bManualVertexFetchSupported(InManualVertexFetchSupported) {}

  virtual void InitRHI(FRHICommandListBase& RHICmdList) override;

private:
  int32 QuadCount;
  bool bManualVertexFetchSupported;
};

/**
 * A dummy vertex buffer to bind when using manual vertex fetch in vertex
 * factories. This prevents rendering pipeline errors that can occur with
 * zero-stream input layouts.
 */
class FCesiumDummyVertexBuffer : public FVertexBuffer {
public:
  virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
};

static TGlobalResource<FCesiumDummyVertexBuffer> GCesiumDummyVertexBuffer;
