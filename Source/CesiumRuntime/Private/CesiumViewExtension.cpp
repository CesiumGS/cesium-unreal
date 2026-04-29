// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumViewExtension.h"

#include "Cesium3DTileset.h"
#include "CesiumCommon.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "DynamicResolutionState.h"
#include "Materials/MaterialRenderProxy.h"
#include "PixelShaderUtils.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneRendering.h"

using namespace Cesium3DTilesSelection;

namespace {
// COPIED FROM SceneRendering.cpp
// static
FIntPoint ApplyResolutionFraction(
    const FSceneViewFamily& ViewFamily,
    const FIntPoint& UnscaledViewSize,
    float ResolutionFraction) {
  FIntPoint ViewSize;

  // CeilToInt so tha view size is at least 1x1 if ResolutionFraction ==
  // ISceneViewFamilyScreenPercentage::kMinResolutionFraction.
  ViewSize.X = FMath::CeilToInt(UnscaledViewSize.X * ResolutionFraction);
  ViewSize.Y = FMath::CeilToInt(UnscaledViewSize.Y * ResolutionFraction);

  check(ViewSize.GetMin() > 0);

  return ViewSize;
}

// static
FIntPoint QuantizeViewRectMin(const FIntPoint& ViewRectMin) {
  FIntPoint Out;

  // Some code paths of Nanite require that view rect is aligned on 8x8
  // boundary.
  static const auto EnableNaniteCVar =
      IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite"));
  const bool bNaniteEnabled =
      (EnableNaniteCVar != nullptr) ? (EnableNaniteCVar->GetInt() != 0) : true;
  const int kMinimumNaniteDivisor = 8; // HTILE size

  QuantizeSceneBufferSize(
      ViewRectMin,
      Out,
      bNaniteEnabled ? kMinimumNaniteDivisor : 0);
  return Out;
}

// static
FIntPoint GetDesiredInternalBufferSize(const FSceneViewFamily& ViewFamily) {
  // If not supporting screen percentage, bypass all computation.
  if (!ViewFamily.SupportsScreenPercentage()) {
    FIntPoint FamilySizeUpperBound(0, 0);

    for (const FSceneView* View : ViewFamily.AllViews) {
      FamilySizeUpperBound.X =
          FMath::Max(FamilySizeUpperBound.X, View->UnscaledViewRect.Max.X);
      FamilySizeUpperBound.Y =
          FMath::Max(FamilySizeUpperBound.Y, View->UnscaledViewRect.Max.Y);
    }

    FIntPoint DesiredBufferSize;
    QuantizeSceneBufferSize(FamilySizeUpperBound, DesiredBufferSize);
    return DesiredBufferSize;
  }

  // Compute final resolution fraction.
  float ResolutionFractionUpperBound = 1.f;
  if (ISceneViewFamilyScreenPercentage const* ScreenPercentageInterface =
          ViewFamily.GetScreenPercentageInterface()) {
    DynamicRenderScaling::TMap<float> DynamicResolutionUpperBounds =
        ScreenPercentageInterface->GetResolutionFractionsUpperBound();
    const float PrimaryResolutionFractionUpperBound =
        DynamicResolutionUpperBounds[GDynamicPrimaryResolutionFraction];
    ResolutionFractionUpperBound =
        PrimaryResolutionFractionUpperBound * ViewFamily.SecondaryViewFraction;
  }

  // TODO CVarLensDistortionffectScreenPercentage not accessible.
  // if (ViewFamily.Views[0]->bIsViewInfo) {
  //  const FViewInfo& View = static_cast<const
  //  FViewInfo&>(*ViewFamily.Views[0]); if (View.LensDistortionLUT.IsEnabled())
  //  {
  //    float AffectScreenPercentage =
  //        CVarLensDistortionAffectScreenPercentage.GetValueOnRenderThread();
  //    ResolutionFractionUpperBound *= FMath::Lerp(
  //        1.0,
  //        View.LensDistortionLUT.ResolutionFraction,
  //        AffectScreenPercentage);
  //  }
  //}

  FIntPoint FamilySizeUpperBound(0, 0);

  // For multiple views, use the maximum overscan fraction to ensure that enough
  // space is allocated so that any overscanned views do not encroach into the
  // space of other views
  float MaxOverscanResolutionFraction = 1.0f;
  for (const FSceneView* View : ViewFamily.AllViews) {
    MaxOverscanResolutionFraction = FMath::Max(
        MaxOverscanResolutionFraction,
        View->SceneViewInitOptions.OverscanResolutionFraction);
  }

  ResolutionFractionUpperBound *= MaxOverscanResolutionFraction;

  for (const FSceneView* View : ViewFamily.AllViews) {
    // Note: This ensures that custom passes (rendered with the main renderer)
    // ignore screen percentage, like regular scene captures.
    const float AdjustedResolutionFractionUpperBounds =
        View->CustomRenderPass
            ? 1.0f
            : (View->SceneViewInitOptions.OverridePrimaryResolutionFraction >
                       0.0
                   ? (View->SceneViewInitOptions
                          .OverridePrimaryResolutionFraction *
                      ViewFamily.SecondaryViewFraction)
                   : ResolutionFractionUpperBound);

    FIntPoint ViewSize = ApplyResolutionFraction(
        ViewFamily,
        View->UnconstrainedViewRect.Size(),
#include "DynamicRenderScaling.h"
        AdjustedResolutionFractionUpperBounds);
    FIntPoint ViewRectMin = QuantizeViewRectMin(FIntPoint(
        FMath::CeilToInt(
            View->UnconstrainedViewRect.Min.X *
            AdjustedResolutionFractionUpperBounds),
        FMath::CeilToInt(
            View->UnconstrainedViewRect.Min.Y *
            AdjustedResolutionFractionUpperBounds)));

    FamilySizeUpperBound.X =
        FMath::Max(FamilySizeUpperBound.X, ViewRectMin.X + ViewSize.X);
    FamilySizeUpperBound.Y =
        FMath::Max(FamilySizeUpperBound.Y, ViewRectMin.Y + ViewSize.Y);
  }

  check(FamilySizeUpperBound.GetMin() > 0);

  FIntPoint DesiredBufferSize;
  QuantizeSceneBufferSize(FamilySizeUpperBound, DesiredBufferSize);

#if !UE_BUILD_SHIPPING
  {
    // Increase the size of desired buffer size by 2 when testing for view
    // rectangle offset.
    static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(
        TEXT("r.Test.ViewRectOffset"));
    if (CVar->GetValueOnAnyThread() > 0) {
      DesiredBufferSize *= 2;
    }
  }
#endif

  return DesiredBufferSize;
}

} // namespace

CesiumViewExtension::CesiumViewExtension(const FAutoRegister& autoRegister)
    : FSceneViewExtensionBase(autoRegister) {}

CesiumViewExtension::~CesiumViewExtension() = default;

void CesiumViewExtension::BeginRenderViewFamily(
    FSceneViewFamily& /*InViewFamily*/) {
  if (!this->_isEnabled)
    return;

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::DequeueOcclusionResults)
  if (!_occlusionResultsQueue.IsEmpty()) {
    // Recycle the current occlusion results.
    for (SceneViewOcclusionResults& occlusionResults :
         _currentOcclusionResults.occlusionResultsByView) {
      occlusionResults.PrimitiveOcclusionResults.Reset();
      _recycledOcclusionResultSets.Enqueue(
          std::move(occlusionResults.PrimitiveOcclusionResults));
    }
    _currentOcclusionResults = {};

    // Update occlusion results from the queue.
    _currentOcclusionResults = std::move(*_occlusionResultsQueue.Peek());
    _occlusionResultsQueue.Pop();
  }
}

namespace {
class FRenderTargetTexture : public FTexture, public FRenderTarget {
public:
  FRenderTargetTexture(FRHITextureCreateDesc InDesc) : Desc(InDesc) {}

  ~FRenderTargetTexture() { this->ReleaseResource(); }

  virtual void InitRHI(FRHICommandListBase& RHICmdList) {
    // Create the sampler state RHI resource.
    FSamplerStateInitializerRHI SamplerStateInitializer(
        SF_Bilinear,
        AM_Clamp,
        AM_Clamp,
        AM_Clamp);
    SamplerStateRHI = GetOrCreateSamplerState(SamplerStateInitializer);

    RenderTargetTextureRHI = TextureRHI = RHICreateTexture(Desc);
  }

  virtual uint32 GetSizeX() const { return Desc.GetSize().X; }

  virtual uint32 GetSizeY() const { return Desc.GetSize().Y; }

  virtual FIntPoint GetSizeXY() const {
    return FIntPoint(GetSizeX(), GetSizeY());
  }

  virtual float GetDisplayGamma() const { return 1.0f; }

  virtual FString GetFriendlyName() const override { return Desc.DebugName; }

private:
  FRHITextureCreateDesc Desc;
};

class FCesiumPrimitiveEdgesViewFamilyData
    : public ISceneViewFamilyExtentionData {
public:
  inline static const TCHAR* const GSubclassIdentifier =
      TEXT("FCesiumPrimitiveEdgesViewFamilyData");
  virtual const TCHAR* GetSubclassIdentifier() const override {
    return GSubclassIdentifier;
  }

  void CreateRenderTargets(
      ERHIFeatureLevel::Type FeatureLevel,
      FIntPoint DesiredBufferSize) {
    int NumMSAASamples = 1;
     //   FSceneTexturesConfig::GetEditorPrimitiveNumSamples(FeatureLevel);

    if (!WireframeColor.IsValid()) {
      FRHITextureCreateDesc ColorDesc =
          FRHITextureCreateDesc::Create2D(TEXT("MeshEdgesRenderTarget"))
              .SetExtent(DesiredBufferSize)
              .SetFormat(PF_B8G8R8A8)
              .SetClearValue(FClearValueBinding::Transparent)
              .SetFlags(
                  ETextureCreateFlags::RenderTargetable |
                  ETextureCreateFlags::ShaderResource)
              .SetInitialState(ERHIAccess::SRVMask)
              .SetNumSamples(NumMSAASamples);
      WireframeColor = MakeUnique<FRenderTargetTexture>(ColorDesc);
    }

    if (!WireframeDepth.IsValid()) {
      FRHITextureCreateDesc DepthDesc =
          FRHITextureCreateDesc::Create2D(TEXT("MeshEdgesDepthRenderTarget"))
              .SetExtent(DesiredBufferSize)
              .SetFormat(PF_DepthStencil)
              .SetClearValue(FClearValueBinding::DepthFar)
              .SetFlags(
                  ETextureCreateFlags::DepthStencilTargetable |
                  ETextureCreateFlags::ShaderResource)
              .SetInitialState(ERHIAccess::SRVMask)
              .SetNumSamples(NumMSAASamples);
      WireframeDepth = MakeUnique<FRenderTargetTexture>(DepthDesc);
    }
  }

  TUniquePtr<FRenderTargetTexture> WireframeColor = nullptr;
  TUniquePtr<FRenderTargetTexture> WireframeDepth = nullptr;
  TArray<FIntRect> ViewRects;
};

void CopyViewFamily(
    const FSceneViewFamily& SrcViewFamily,
    FSceneViewFamily& ViewFamily) {
  ViewFamily.FrameNumber = SrcViewFamily.FrameNumber;
  ViewFamily.FrameCounter = SrcViewFamily.FrameCounter;
  ViewFamily.ViewExtensions = GEngine->ViewExtensions->GatherActiveExtensions(
      FSceneViewExtensionContext(SrcViewFamily.Scene));

  for (int32 ViewIndex = 0; ViewIndex < SrcViewFamily.Views.Num();
       ++ViewIndex) {
    const FSceneView* SrcSceneView = SrcViewFamily.Views[ViewIndex];
    if (ensure(SrcSceneView)) {
      FSceneViewInitOptions ViewInitOptions =
          SrcSceneView->SceneViewInitOptions;
      ViewInitOptions.ViewFamily = &ViewFamily;
      ViewInitOptions.ViewLocation = SrcSceneView->ViewLocation;
      ViewInitOptions.ViewRotation = SrcSceneView->ViewRotation;

      // Reset to avoid incorrect culling problems
      ViewInitOptions.SceneViewStateInterface =
          FSceneViewInitOptions{}.SceneViewStateInterface;

      FSceneView* View = new FSceneView(ViewInitOptions);
      ViewFamily.Views.Emplace(View);
    }
  }
}

void RenderMeshEdges_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FSceneRenderer* SceneRenderer,
    FCesiumPrimitiveEdgesViewFamilyData& MeshEdgesData) {
  MeshEdgesData.WireframeColor->InitResource(RHICmdList);
  MeshEdgesData.WireframeDepth->InitResource(RHICmdList);

  SceneRenderer->RenderThreadBegin(RHICmdList);

  FUniformExpressionCacheAsyncUpdateScope AsyncUpdateScope;

  // update any resources that needed a deferred update
  FDeferredUpdateResource::UpdateResources(RHICmdList);

  FRDGBuilder GraphBuilder(
      RHICmdList,
      RDG_EVENT_NAME("CesiumMeshEdges"),
      ERDGBuilderFlags::Parallel);
  {
    RDG_EVENT_SCOPE(GraphBuilder, "CesiumRenderMeshEdges_RenderThread");

    // Render the scene normally
    {
      RDG_RHI_EVENT_SCOPE(GraphBuilder, RenderScene);
      SceneRenderer->Render(GraphBuilder);
    }
  }
  GraphBuilder.Execute();

  for (const FViewInfo& ViewInfo : SceneRenderer->Views) {
    MeshEdgesData.ViewRects.Emplace(ViewInfo.ViewRect);
  }

  SceneRenderer->RenderThreadEnd(RHICmdList);
}

void RenderMeshEdges(FSceneViewFamily& ViewFamily) {
  if (ViewFamily.EngineShowFlags.Wireframe) {
    return;
  }

  FCesiumPrimitiveEdgesViewFamilyData* ViewFamilyData =
      ViewFamily
          .GetOrCreateExtentionData<FCesiumPrimitiveEdgesViewFamilyData>();

  ERHIFeatureLevel::Type FeatureLevel = ViewFamily.GetFeatureLevel();
  FIntPoint DesiredBufferSize = GetDesiredInternalBufferSize(ViewFamily);
  ViewFamilyData->CreateRenderTargets(FeatureLevel, DesiredBufferSize);

  FEngineShowFlags WireframeShowFlags = ViewFamily.EngineShowFlags;
  {
    // Render a wireframe view
    WireframeShowFlags.SetWireframe(true);

    // Copy the MSAA wireframe view only, don't copy other scene elements
    WireframeShowFlags.SetSceneCaptureCopySceneDepth(false);

    // Disable rendering of elements that are not needed
    WireframeShowFlags.SetMeshEdges(false);
    WireframeShowFlags.SetLighting(false);
    WireframeShowFlags.SetLightFunctions(false);
    WireframeShowFlags.SetGlobalIllumination(false);
    WireframeShowFlags.SetLumenGlobalIllumination(false);
    WireframeShowFlags.SetLumenReflections(false);
    WireframeShowFlags.SetDynamicShadows(false);
    WireframeShowFlags.SetCapsuleShadows(false);
    WireframeShowFlags.SetDistanceFieldAO(false);
    WireframeShowFlags.SetFog(false);
    WireframeShowFlags.SetVolumetricFog(false);
    WireframeShowFlags.SetCloud(false);
    WireframeShowFlags.SetDecals(false);
    WireframeShowFlags.SetAtmosphere(false);
    WireframeShowFlags.SetPostProcessing(false);
    WireframeShowFlags.SetCompositeEditorPrimitives(false);
    WireframeShowFlags.SetGrid(false);
    WireframeShowFlags.SetShaderPrint(false);
    // WireframeShowFlags.SetScreenPercentage(false);
    // WireframeShowFlags.SetTranslucency(false);
  }

  FSceneViewFamilyContext CaptureViewFamily(
      FSceneViewFamily::ConstructionValues(
          ViewFamilyData->WireframeColor.Get(),
          ViewFamily.Scene,
          WireframeShowFlags)
          .SetRenderTargetDepth(ViewFamilyData->WireframeDepth.Get())
          .SetResolveScene(true)
          .SetRealtimeUpdate(true)
          .SetTime(ViewFamily.Time));

  {
    CopyViewFamily(ViewFamily, CaptureViewFamily);

    CaptureViewFamily.SceneCaptureSource = SCS_SceneColorSceneDepth;

    // Use the same resolution scale as main view, so the buffers align
    // pixel-perfect. If the main view is low-res this affects the wireframe
    // quality, so the main view should be 100% ideally
    CaptureViewFamily.SetScreenPercentageInterface(
        ViewFamily.GetScreenPercentageInterface()->Fork_GameThread(
            CaptureViewFamily));
  }

  // THIS RESULTS IN A RECURSIVE CALL OF POSTCREATESCENERENDERER. THERE HAS TO
  // BE A BETTER WAY TO DO THIS!
  FSceneRenderer* SceneRenderer =
      FSceneRenderer::CreateSceneRenderer(&CaptureViewFamily, nullptr);

  for (const FSceneViewExtensionRef& Extension :
       CaptureViewFamily.ViewExtensions) {
    Extension->SetupViewFamily(CaptureViewFamily);
  }

  for (int32 ViewIndex = 0; ViewIndex < SceneRenderer->Views.Num();
       ++ViewIndex) {
    FViewInfo& ViewInfo = SceneRenderer->Views[ViewIndex];
    ViewInfo.bAllowTemporalJitter = false;
    ViewInfo.PrimaryScreenPercentageMethod =
        EPrimaryScreenPercentageMethod::RawOutput;

    for (const FSceneViewExtensionRef& Extension :
         CaptureViewFamily.ViewExtensions) {
      Extension->SetupView(CaptureViewFamily, ViewInfo);
    }
  }

  ENQUEUE_RENDER_COMMAND(CaptureCommand)
  ([SceneRenderer, ViewFamilyData](FRHICommandListImmediate& RHICmdList) {
    RenderMeshEdges_RenderThread(RHICmdList, SceneRenderer, *ViewFamilyData);
  });
}

class FCesiumComposePrimitiveEdgesPS : public FGlobalShader {
  DECLARE_GLOBAL_SHADER(FCesiumComposePrimitiveEdgesPS);
  SHADER_USE_PARAMETER_STRUCT(FCesiumComposePrimitiveEdgesPS, FGlobalShader)

  static const uint32 kMSAASampleCountMaxLog2 = 3; // = log2(MSAASampleCountMax)
  static const uint32 kMSAASampleCountMax = 1 << kMSAASampleCountMaxLog2;
  // class FSampleCountDimension : SHADER_PERMUTATION_RANGE_INT(
  //                                   "MSAA_SAMPLE_COUNT_LOG2",
  //                                   0,
  //                                   kMSAASampleCountMaxLog2 + 1);
  // using FPermutationDomain = TShaderPermutationDomain<FSampleCountDimension>;

  static bool ShouldCompilePermutation(
      const FGlobalShaderPermutationParameters& Parameters) {
    return FDataDrivenShaderPlatformInfo::GetSupportsDebugViewShaders(
        Parameters.Platform);
  }

  static void ModifyCompilationEnvironment(
      const FGlobalShaderPermutationParameters& Parameters,
      FShaderCompilerEnvironment& OutEnvironment) {
    const FPermutationDomain PermutationVector(Parameters.PermutationId);
    const int32 SampleCount = 1;
    //   << PermutationVector.Get<FSampleCountDimension>();
    OutEnvironment.SetDefine(TEXT("MSAA_SAMPLE_COUNT"), SampleCount);
  }

  BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
  SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WireframeColorTexture)
  SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WireframeDepthTexture)
  SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Wireframe)

  SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTexture)
  SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)
  SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Depth)
  SHADER_PARAMETER(FVector2f, DepthTextureJitter)
  SHADER_PARAMETER_ARRAY(
      FVector4f,
      SampleOffsetArray,
      [FCesiumComposePrimitiveEdgesPS::kMSAASampleCountMax])

  SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, Output)

  RENDER_TARGET_BINDING_SLOTS()
  END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(
    FCesiumComposePrimitiveEdgesPS,
    "/Plugin/CesiumForUnreal/Private/CesiumMeshEdges.usf",
    "CesiumComposePrimitiveEdgesPS",
    SF_Pixel);

void ComposeMeshEdges(
    FRDGBuilder& GraphBuilder,
    FSceneView& View,
    const FRenderTargetBindingSlots& RenderTargets,
    TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) {
  const FSceneViewFamily& ViewFamily = *View.Family;

  int ViewIndex = 0;
  for (; ViewIndex < ViewFamily.Views.Num(); ViewIndex++) {
    if (ViewFamily.Views[ViewIndex] == &View)
      break;
  }

  const FCesiumPrimitiveEdgesViewFamilyData* ViewFamilyData =
      ViewFamily.GetExtentionData<FCesiumPrimitiveEdgesViewFamilyData>();
  if (!ViewFamilyData) {
    return;
  }

  const FIntRect Viewport = static_cast<const FViewInfo&>(View).ViewRect;
  FRenderTargetTexture& WireframeTextureColor = *ViewFamilyData->WireframeColor;
  FRenderTargetTexture& WireframeTextureDepth = *ViewFamilyData->WireframeDepth;
  const FIntRect& WireframeViewRect = ViewFamilyData->ViewRects[ViewIndex];
  FRDGTexture* pDepthTexture = SceneTextures->GetContents()->SceneDepthTexture;
  FScreenPassTextureViewport SceneDepth(pDepthTexture);
  FScreenPassTextureViewport Output(RenderTargets[0].GetTexture());

  FRHISamplerState* PointClampSampler =
      TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

  FCesiumComposePrimitiveEdgesPS::FParameters* PassParameters =
      GraphBuilder
          .AllocParameters<FCesiumComposePrimitiveEdgesPS::FParameters>();
  PassParameters->WireframeColorTexture = RegisterExternalTexture(
      GraphBuilder,
      WireframeTextureColor.TextureRHI,
      *WireframeTextureColor.GetFriendlyName());
  PassParameters->WireframeDepthTexture = RegisterExternalTexture(
      GraphBuilder,
      WireframeTextureDepth.TextureRHI,
      *WireframeTextureDepth.GetFriendlyName());
  PassParameters->Wireframe = GetScreenPassTextureViewportParameters(
      FScreenPassTextureViewport(WireframeViewRect));
  PassParameters->Depth = GetScreenPassTextureViewportParameters(SceneDepth);
  PassParameters->Output = GetScreenPassTextureViewportParameters(Output);
  PassParameters->DepthTexture = pDepthTexture;
  PassParameters->DepthSampler = PointClampSampler;
  //  PassParameters->DepthTextureJitter = FVector2f(View.);

  // for (int32 i = 0; i < int32(NumMSAASamples); i++) {
  //   PassParameters->SampleOffsetArray[i].X =
  //       GetMSAASampleOffsets(NumMSAASamples, i).X;
  //   PassParameters->SampleOffsetArray[i].Y =
  //       GetMSAASampleOffsets(NumMSAASamples, i).Y;
  // }

  PassParameters->RenderTargets[0] = RenderTargets.Output[0];
  PassParameters->RenderTargets.DepthStencil = RenderTargets.DepthStencil;

  // const int MSAASampleCountDim = FMath::FloorLog2(NumMSAASamples);

  // FCesiumComposePrimitiveEdgesPS::FPermutationDomain PermutationVector;
  // PermutationVector.Set<FCesiumComposePrimitiveEdgesPS::FSampleCountDimension>(
  //     MSAASampleCountDim);

  const FGlobalShaderMap* GlobalShaderMap =
      GetGlobalShaderMap(View.GetFeatureLevel());
  const TShaderRef<FCesiumComposePrimitiveEdgesPS>& PixelShader =
      TShaderMapRef<FCesiumComposePrimitiveEdgesPS>(GlobalShaderMap);

  FRHIBlendState* BlendState = TStaticBlendState<CW_RGBA>::GetRHI();
  FRHIDepthStencilState* DepthStencilState =
      TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI();

  FPixelShaderUtils::AddFullscreenPass(
      GraphBuilder,
      GlobalShaderMap,
      RDG_EVENT_NAME("FCesiumComposePrimitiveEdgesPS"),
      PixelShader,
      PassParameters,
      Viewport,
      BlendState,
      nullptr,
      DepthStencilState);

  GraphBuilder.AddPostExecuteCallback(
      [&WireframeTextureColor, &WireframeTextureDepth] {
        WireframeTextureColor.ReleaseResource();
        WireframeTextureDepth.ReleaseResource();
      });
}
} // namespace

void CesiumViewExtension::PostCreateSceneRenderer(
    const FSceneViewFamily& InViewFamily,
    ISceneRenderer* Renderer) {
  if (this->_componentsWithEdgeVisibility.Num()) {
    RenderMeshEdges(static_cast<FSceneRenderer*>(Renderer)->ViewFamily);
  }
}

void CesiumViewExtension::PreRenderView_RenderThread(
    FRDGBuilder& GraphBuilder,
    FSceneView& InView) {
  /*GraphBuilder.AddPass(
      FRDGEventName(TEXT("EXT_mesh_primitive_edge_visibility Prepass")),
      ERDGPassFlags::Raster,
      [&InView, components = this->_componentsWithEdgeVisibility](
          FRDGAsyncTask,
          FRHICommandList& RHICmdList) {
        for (UCesiumGltfPrimitiveComponent* pComponent : components) {
          if (!IsValid(pComponent) || pComponent->IsBeingDestroyed() ||
              !pComponent->IsVisible()) {
            continue;
          }
          pComponent->GetSceneProxy();

          TObjectPtr<UStaticMesh> pMesh =
              pComponent->GetMeshComponent().GetStaticMesh();
          if (!IsValid(pMesh)) {
            continue;
          }

        }
      });*/
}

TileOcclusionState CesiumViewExtension::getPrimitiveOcclusionState(
    const FPrimitiveComponentId& id,
    bool previouslyOccluded,
    float frameTimeCutoff) const {
  if (_currentOcclusionResults.occlusionResultsByView.size() == 0) {
    return TileOcclusionState::OcclusionUnavailable;
  }

  bool isOccluded = false;
  bool historyMissing = false;

  for (const SceneViewOcclusionResults& viewOcclusionResults :
       _currentOcclusionResults.occlusionResultsByView) {
    const PrimitiveOcclusionResult* pOcclusionResult =
        viewOcclusionResults.PrimitiveOcclusionResults.Find(id);

    if (pOcclusionResult &&
        pOcclusionResult->LastConsideredTime >= frameTimeCutoff) {
      if (!pOcclusionResult->OcclusionStateWasDefiniteLastFrame) {
        return TileOcclusionState::OcclusionUnavailable;
      }

      if (previouslyOccluded) {
        if (pOcclusionResult->LastPixelsPercentage > 0.01f) {
          return TileOcclusionState::NotOccluded;
        }
      } else if (!pOcclusionResult->WasOccludedLastFrame) {
        return TileOcclusionState::NotOccluded;
      }

      isOccluded = true;
    } else {
      historyMissing = true;
    }
  }

  if (historyMissing) {
    return TileOcclusionState::OcclusionUnavailable;
  } else if (isOccluded) {
    return TileOcclusionState::Occluded;
  } else {
    return TileOcclusionState::NotOccluded;
  }
}

namespace {

const TSet<FPrimitiveOcclusionHistory, FPrimitiveOcclusionHistoryKeyFuncs>&
getOcclusionHistorySet(const FSceneViewState* pViewState) {
  return pViewState->Occlusion.PrimitiveOcclusionHistorySet;
}

} // namespace

void CesiumViewExtension::PostRenderViewFamily_RenderThread(
    FRDGBuilder& GraphBuilder,
    FSceneViewFamily& InViewFamily) {
  if (!this->_isEnabled)
    return;
  if (_frameNumber_renderThread != InViewFamily.FrameNumber) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EnqueueAggregatedOcclusion)
    if (_frameNumber_renderThread != -1) {
      _occlusionResultsQueue.Enqueue(
          std::move(_currentAggregation_renderThread));
      _currentAggregation_renderThread = {};
    }

    _frameNumber_renderThread = InViewFamily.FrameNumber;
  }

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::AggregateOcclusionForViewFamily)
  for (const FSceneView* pView : InViewFamily.Views) {
    if (pView == nullptr || pView->State == nullptr)
      continue;

    const FSceneViewState* pViewState = pView->State->GetConcreteViewState();
    if (pViewState && getOcclusionHistorySet(pViewState).Num()) {
      SceneViewOcclusionResults& occlusionResults =
          _currentAggregation_renderThread.occlusionResultsByView
              .emplace_back();
      // Do we actually need the view?
      occlusionResults.pView = pView;

      if (!_recycledOcclusionResultSets.IsEmpty()) {
        // Recycle a previously allocated occlusion history set, if one is
        // available.
        occlusionResults.PrimitiveOcclusionResults =
            std::move(*_recycledOcclusionResultSets.Peek());
        _recycledOcclusionResultSets.Pop();
      } else {
        // If no previously-allocated set exists, just allocate a new one. It
        // will be recycled later.
      }

      occlusionResults.PrimitiveOcclusionResults.Reserve(
          getOcclusionHistorySet(pViewState).Num());
      for (const auto& element : getOcclusionHistorySet(pViewState)) {
        occlusionResults.PrimitiveOcclusionResults.Emplace(element);
      }

      // Unreal will not execute occlusion queries that get frustum culled in a
      // particular view, leaving the occlusion results indefinite. And by just
      // looking at the PrimitiveOcclusionHistorySet, we can't distinguish
      // occlusion queries that haven't completed "yet" from occlusion queries
      // that were culled. So here we detect primitives that have been
      // conclusively proven to be not visible (outside the view frustum) and
      // also mark them definitely occluded.
      FScene* pScene = InViewFamily.Scene->GetRenderScene();
      if (pView->bIsViewInfo && pScene != nullptr) {
        const FViewInfo* pViewInfo = static_cast<const FViewInfo*>(pView);
        const FSceneBitArray& visibility = pViewInfo->PrimitiveVisibilityMap;
        auto& occlusion = occlusionResults.PrimitiveOcclusionResults;

        const uint32 PrimitiveCount = pScene->Primitives.Num();
        for (uint32 i = 0; i < PrimitiveCount; ++i) {
          FPrimitiveSceneInfo* pSceneInfo = pScene->Primitives[i];
          if (pSceneInfo == nullptr)
            continue;

          bool setOcclusionState = false;
          bool isOccluded = false;

          // Unreal will never compute occlusion for primitives that are
          // selected in the Editor. So treat these as unoccluded.
#if WITH_EDITOR
          if (GIsEditor) {
            if (pScene->PrimitivesSelected[i]) {
              setOcclusionState = true;
              isOccluded = false;
            }
          }
#endif

          // If this primitive is not visible at all (and also not selected!),
          // treat is as occluded.
          if (!setOcclusionState && !visibility[i]) {
            setOcclusionState = true;
            isOccluded = true;
          }

          if (setOcclusionState) {
            const PrimitiveOcclusionResult* pOcclusionResult =
                occlusion.Find(pSceneInfo->PrimitiveComponentId);
            if (!pOcclusionResult || pOcclusionResult->LastConsideredTime <
                                         pViewState->LastRenderTime) {
              // No valid occlusion history for this culled primitive, so create
              // it.
              occlusion.Emplace(PrimitiveOcclusionResult(
                  pSceneInfo->PrimitiveComponentId,
                  pViewState->LastRenderTime,
                  isOccluded ? 0.0f : 100.0f,
                  true,
                  isOccluded));
            }
          }
        }
      }
    }
  }
}

void CesiumViewExtension::PostRenderBasePassDeferred_RenderThread(
    FRDGBuilder& GraphBuilder,
    FSceneView& InView,
    const FRenderTargetBindingSlots& RenderTargets,
    TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) {
  if (this->_componentsWithEdgeVisibility.Num()) {
    ComposeMeshEdges(GraphBuilder, InView, RenderTargets, SceneTextures);
  }
}

void CesiumViewExtension::SubscribeToPostProcessingPass(
    EPostProcessingPass Pass,
    const FSceneView& InView,
    FAfterPassCallbackDelegateArray& InOutPassCallbacks,
    bool bIsPassEnabled) {}

void CesiumViewExtension::SetEnabled(bool enabled) {
  this->_isEnabled = enabled;
}

void CesiumViewExtension::registerComponentWithEdges(
    UCesiumGltfPrimitiveComponent* pComponent) {
  this->_componentsWithEdgeVisibility.Add(pComponent);
}
void CesiumViewExtension::unregisterComponentWithEdges(
    UCesiumGltfPrimitiveComponent* pComponent) {
  this->_componentsWithEdgeVisibility.Remove(pComponent);
}
