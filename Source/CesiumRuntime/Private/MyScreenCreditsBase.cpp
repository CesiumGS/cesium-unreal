// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "MyScreenCreditsBase.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "CesiumCreditSystem.h"
#include "Components/RichTextBlock.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Base64.h"
#include "Rendering/DrawElements.h"
#include "Slate/SlateGameResources.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SRichTextHyperlink.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SCompoundWidget.h"
#include <string>
#include <tidybuffio.h>
#include <vector>

using namespace Cesium3DTilesSelection;

class SRichInlineImage : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SRichInlineImage) {}
  SLATE_END_ARGS()

public:
  void Construct(
      const FArguments& InArgs,
      const FSlateBrush* Brush,
      const FString& url,
      const FString& text,
      const FTextBlockStyle& TextStyle,
      FOnPopupClicked* InEventHandler) {
    if (Brush) {
      const TSharedRef<FSlateFontMeasure> FontMeasure =
          FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
      float IconHeight = FMath::Min(
          (float)FontMeasure->GetMaxCharacterHeight(TextStyle.Font, 2.0f),
          Brush->ImageSize.Y);

      float IconWidth =
          IconHeight / (float)Brush->ImageSize.Y * Brush->ImageSize.X;
      ChildSlot
          [SNew(SBox)
               .HeightOverride(IconHeight)
               .WidthOverride(IconWidth)
               .VAlign(VAlign_Center)
                   [SNew(SImage).Image(Brush).OnMouseButtonDown_Lambda(
                       [url](const FGeometry&, const FPointerEvent&) -> FReply {
                         FPlatformProcess::LaunchURL(*url, NULL, NULL);
                         return FReply::Handled();
                       })]];
    } else {
      TSharedPtr<FSlateHyperlinkRun::FWidgetViewModel> model =
          MakeShareable(new FSlateHyperlinkRun::FWidgetViewModel);

      ChildSlot[SNew(SRichTextHyperlink, model.ToSharedRef())
                    .Text(FText::FromString(text))
                    .OnNavigate_Lambda([url, InEventHandler]() {
                      if (url.Equals("popup")) {
                        InEventHandler->Execute();
                      } else {
                        FPlatformProcess::LaunchURL(*url, NULL, NULL);
                      }
                    })];
    }
  }
};

class FRichInlineImage : public FRichTextDecorator {
public:
  FRichInlineImage(
      URichTextBlock* InOwner,
      UMyRichTextBlockDecorator* InDecorator)
      : FRichTextDecorator(InOwner), Decorator(InDecorator) {}

  virtual bool Supports(
      const FTextRunParseResults& RunParseResult,
      const FString& Text) const override {
    if (RunParseResult.Name == TEXT("img") &&
            RunParseResult.MetaData.Contains(TEXT("id")) ||
        RunParseResult.MetaData.Contains(TEXT("url"))) {
      return true;
    }
    return false;
  }

protected:
  virtual TSharedPtr<SWidget> CreateDecoratorWidget(
      const FTextRunInfo& RunInfo,
      const FTextBlockStyle& TextStyle) const override {
    FString url;
    FString text;
    const FSlateBrush* Brush = nullptr;
    if (RunInfo.MetaData.Contains(TEXT("url"))) {
      url = *RunInfo.MetaData[TEXT("url")];
    }
    if (RunInfo.MetaData.Contains(TEXT("text"))) {
      text = *RunInfo.MetaData[TEXT("text")];
    }
    if (RunInfo.MetaData.Contains(TEXT("id"))) {
      int32 id = FCString::Atoi(*RunInfo.MetaData[TEXT("id")]);
      Brush = Decorator->FindImageBrush(id);
    }
    TSharedPtr<SWidget> ret = SNew(
        SRichInlineImage,
        Brush,
        url,
        text,
        TextStyle,
        &Decorator->EventHandler);
    return ret;
  }

private:
  UMyRichTextBlockDecorator* Decorator;
};

UMyRichTextBlockDecorator::UMyRichTextBlockDecorator(
    const FObjectInitializer& ObjectInitializer)
    : URichTextBlockDecorator(ObjectInitializer) {}

TSharedPtr<ITextDecorator>
UMyRichTextBlockDecorator::CreateDecorator(URichTextBlock* InOwner) {
  return MakeShareable(new FRichInlineImage(InOwner, this));
}

const FSlateBrush* UMyRichTextBlockDecorator::FindImageBrush(int32 id) {
  if (_textureResources.Num() > id) {
    return _textureResources[id];
  }
  return nullptr;
}

UMyScreenCreditsBase::UMyScreenCreditsBase(
    const FObjectInitializer& ObjectInitializer)
    : UUserWidget(ObjectInitializer) {}

void UMyScreenCreditsBase::OnPopupClicked() { _showPopup ^= _showPopup; }

void UMyScreenCreditsBase::NativeConstruct() {
  if (RichTextBlock_127) {
    _imageDecorator = static_cast<UMyRichTextBlockDecorator*>(
        RichTextBlock_127->GetDecoratorByClass(
            UMyRichTextBlockDecorator::StaticClass()));

    _imageDecorator->EventHandler.BindUObject(
        this,
        &UMyScreenCreditsBase::OnPopupClicked);
  }
  UWorld* world = GetWorld();
  if (world) {
    _pCreditSystem = ACesiumCreditSystem::GetDefaultCreditSystem(GetWorld())
                         ->GetExternalCreditSystem();
  }
}

void UMyScreenCreditsBase::HandleImageRequest(
    FHttpRequestPtr HttpRequest,
    FHttpResponsePtr HttpResponse,
    bool bSucceeded,
    int32 id) {
  if (bSucceeded && HttpResponse.IsValid() &&
      HttpResponse->GetContentLength() > 0) {
    UTexture2D* Texture =
        FImageUtils::ImportBufferAsTexture2D(HttpResponse->GetContent());
    Texture->SRGB = true;
    Texture->UpdateResource();
    Texture->AddToRoot();
    _imageDecorator->_textureResources[id] = new FSlateDynamicImageBrush(
        Texture,
        FVector2D(Texture->PlatformData->SizeX, Texture->PlatformData->SizeY),
        FName(HttpRequest->GetURL()));
    Invalidate(EInvalidateWidgetReason::Layout);
    _output += TEXT('\u200B');
    RichTextBlock_127->SetText(FText::FromString(_output));
    return;
  }
}

std::string UMyScreenCreditsBase::LoadImage(const std::string& url) {

  const std::string base_64_prefix = "data:image/png;base64,";

  if (url.rfind(base_64_prefix, 0) == 0) {
    TArray<uint8> data_buffer;
    FString base64 = UTF8_TO_TCHAR(url.c_str() + base_64_prefix.length());
    if (FBase64::Decode(base64, data_buffer)) {
      UTexture2D* Texture = FImageUtils::ImportBufferAsTexture2D(data_buffer);
      Texture->SRGB = true;
      Texture->UpdateResource();
      Texture->AddToRoot();
      _imageDecorator->_textureResources.Add(new FSlateDynamicImageBrush(
          Texture,
          FVector2D(Texture->PlatformData->SizeX, Texture->PlatformData->SizeY),
          "Untitled"));
    }
  } else {
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest =
        FHttpModule::Get().CreateRequest();

    _imageDecorator->_textureResources.Add(nullptr);
    HttpRequest->OnProcessRequestComplete().BindUObject(
        this,
        &UMyScreenCreditsBase::HandleImageRequest,
        _imageDecorator->_textureResources.Num() - 1);

    HttpRequest->SetURL(UTF8_TO_TCHAR(url.c_str()));
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->ProcessRequest();
  }
  return std::to_string(_imageDecorator->_textureResources.Num() - 1);
}

namespace {
void ConvertHTMLToRTF(
    std::string& output,
    std::string& parentUrl,
    TidyDoc tdoc,
    TidyNode tnod,
    UMyScreenCreditsBase* base) {
  TidyNode child;
  for (child = tidyGetChild(tnod); child; child = tidyGetNext(child)) {
    if (tidyNodeIsText(child)) {
      TidyBuffer buf;
      tidyBufInit(&buf);
      tidyNodeGetText(tdoc, child, &buf);
      if (buf.bp) {
        std::string text = reinterpret_cast<const char*>(buf.bp);
        // could not find correct option in tidy html to not add new lines
        if (text.size() != 0 && text[text.size() - 1] == '\n') {
          text.pop_back();
        }
        if (!parentUrl.empty()) {
          output +=
              "<img url=\"" + parentUrl + "\"" + " text=\"" + text + "\"/>";
        } else {
          output += text;
        }
      }
      tidyBufFree(&buf);
    } else if (tidyNodeGetId(child) == TidyTagId::TidyTag_IMG) {
      auto srcAttr = tidyAttrGetById(child, TidyAttrId::TidyAttr_SRC);
      if (srcAttr) {
        auto srcValue = tidyAttrValue(srcAttr);
        if (srcValue) {
          output += "<img id=\"" +
                    base->LoadImage(
                        std::string(reinterpret_cast<const char*>(srcValue))) +
                    "\"";
          if (!parentUrl.empty()) {
            output += " url=\"" + parentUrl + "\"";
          }
          output += "/>";
        }
      }
    }
    auto hrefAttr = tidyAttrGetById(child, TidyAttrId::TidyAttr_HREF);
    if (hrefAttr) {
      auto hrefValue = tidyAttrValue(hrefAttr);
      parentUrl = std::string(reinterpret_cast<const char*>(hrefValue));
    }
    ConvertHTMLToRTF(output, parentUrl, tdoc, child, base);
  }
}
} // namespace

FString UMyScreenCreditsBase::ConvertCreditToRTF(
    const Cesium3DTilesSelection::Credit* credit) {

  std::string html = _pCreditSystem->getHtml(*credit);
  TidyDoc tdoc;
  TidyBuffer docbuf = {0};
  TidyBuffer tidy_errbuf = {0};
  int err;

  tdoc = tidyCreate();
  tidyOptSetBool(tdoc, TidyForceOutput, yes);
  tidyOptSetInt(tdoc, TidyWrapLen, 0);
  tidyOptSetInt(tdoc, TidyNewline, TidyLF);

  tidySetErrorBuffer(tdoc, &tidy_errbuf);
  tidyBufInit(&docbuf);

  // avoid buffer overflow by wrapping with html tags
  html = "<span>" + html + "</span>";

  tidyBufAppend(
      &docbuf,
      reinterpret_cast<void*>(const_cast<char*>(html.c_str())),
      static_cast<uint>(html.size()));

  err = tidyParseBuffer(tdoc, &docbuf);

  std::string output, url;
  ConvertHTMLToRTF(output, url, tdoc, tidyGetRoot(tdoc), this);

  tidyBufFree(&docbuf);
  tidyBufFree(&tidy_errbuf);
  tidyRelease(tdoc);
  return UTF8_TO_TCHAR(output.c_str());
}

void UMyScreenCreditsBase::Update() {
  if (!_pCreditSystem) {
    return;
  }

  const std::vector<Cesium3DTilesSelection::Credit>& creditsToShowThisFrame =
      _pCreditSystem->getCreditsToShowThisFrame();

  bool CreditsUpdated =
      creditsToShowThisFrame.size() != _lastCreditsCount ||
      _pCreditSystem->getCreditsToNoLongerShowThisFrame().size() > 0;

  if (CreditsUpdated) {
    _lastCreditsCount = creditsToShowThisFrame.size();
    _output.Reset();

    for (int i = 0; i < creditsToShowThisFrame.size(); i++) {
      if (i != 0) {
        _output += " ";
      }
      const Credit* credit = &creditsToShowThisFrame[i];
      if (_creditToRTF.Contains(credit)) {
        _output += _creditToRTF[credit];
      } else {
        FString convert = ConvertCreditToRTF(credit);
        _creditToRTF.Add(credit, convert);
        _output += convert;
      }
    }
    if (RichTextBlock_127) {
      _output += "<img url=\"popup\" text=\" Data attribution\"/>";
      RichTextBlock_127->SetText(FText::FromString(_output));
    }
  }
  _pCreditSystem->startNextFrame();
}
