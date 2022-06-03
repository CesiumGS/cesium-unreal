// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "ScreenCreditsWidget.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "CesiumCreditSystem.h"
#include "Components/BackgroundBlur.h"
#include "Components/RichTextBlock.h"
#include "Engine/Font.h"
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
      UCreditsDecorator* InDecorator) {
    if (Brush) {
      float IconHeight, IconWidth;
      if (InDecorator->_shrinkImageSize) {
        const TSharedRef<FSlateFontMeasure> FontMeasure =
            FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        IconHeight = FMath::Min(
            (float)FontMeasure->GetMaxCharacterHeight(TextStyle.Font, 2.0f),
            Brush->ImageSize.Y);
        IconWidth = IconHeight / (float)Brush->ImageSize.Y * Brush->ImageSize.X;
      } else {
        IconHeight = Brush->ImageSize.Y;
        IconWidth = Brush->ImageSize.X;
      }
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
                    .OnNavigate_Lambda([url, InDecorator]() {
                      if (url.Equals("popup")) {
                        InDecorator->EventHandler.Execute();
                      } else {
                        FPlatformProcess::LaunchURL(*url, NULL, NULL);
                      }
                    })];
    }
  }
};

class FRichInlineImage : public FRichTextDecorator {
public:
  FRichInlineImage(URichTextBlock* InOwner, UCreditsDecorator* InDecorator)
      : FRichTextDecorator(InOwner), Decorator(InDecorator) {}

  virtual bool Supports(
      const FTextRunParseResults& RunParseResult,
      const FString& Text) const override {
    if (RunParseResult.Name == TEXT("credits") &&
        (RunParseResult.MetaData.Contains(TEXT("id")) ||
         RunParseResult.MetaData.Contains(TEXT("url")))) {
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
    TSharedPtr<SWidget> ret =
        SNew(SRichInlineImage, Brush, url, text, TextStyle, Decorator);
    return ret;
  }

private:
  UCreditsDecorator* Decorator;
};

UCreditsDecorator::UCreditsDecorator(
    const FObjectInitializer& ObjectInitializer)
    : URichTextBlockDecorator(ObjectInitializer) {}

TSharedPtr<ITextDecorator>
UCreditsDecorator::CreateDecorator(URichTextBlock* InOwner) {
  return MakeShareable(new FRichInlineImage(InOwner, this));
}

const FSlateBrush* UCreditsDecorator::FindImageBrush(int32 id) {
  if (ScreenBase->_textureResources.Num() > id) {
    return ScreenBase->_textureResources[id];
  }
  return nullptr;
}

UScreenCreditsWidget::UScreenCreditsWidget(
    const FObjectInitializer& ObjectInitializer)
    : UUserWidget(ObjectInitializer) {
  static ConstructorHelpers::FObjectFinder<UFont> RobotoFontObj(
      *UWidget::GetDefaultFontName());
  Font = FSlateFontInfo(RobotoFontObj.Object, 8);
}

void UScreenCreditsWidget::OnPopupClicked() {
  _showPopup = !_showPopup;
  if (_showPopup) {
    BackgroundBlur->SetVisibility(ESlateVisibility::Visible);

    const std::vector<Cesium3DTilesSelection::Credit>& creditsToShowThisFrame =
        _pCreditSystem->getCreditsToShowThisFrame();

    FString output;

    for (int i = 0; i < creditsToShowThisFrame.size(); i++) {
      const Credit* credit = &creditsToShowThisFrame[i];
      if (i != 0) {
        output += "\n";
      }
      if (_creditToRTF.Contains(credit)) {
        output += _creditToRTF[credit];
      } else {
        FString convert = ConvertCreditToRTF(credit);
        _creditToRTF.Add(credit, convert);
        output += convert;
      }
    }
    if (RichTextOnScreen) {
      RichTextPopup->SetText(FText::FromString(output));
    }
  } else {
    BackgroundBlur->SetVisibility(ESlateVisibility::Collapsed);
  }
}

void UScreenCreditsWidget::NativeConstruct() {

  Super::NativeConstruct();

  auto white = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f));

  if (RichTextOnScreen) {
    RichTextOnScreen->SetDefaultFont(Font);
    RichTextOnScreen->SetDefaultColorAndOpacity(
        FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f)));
    _imageDecoratorOnScreen =
        static_cast<UCreditsDecorator*>(RichTextOnScreen->GetDecoratorByClass(
            UCreditsDecorator::StaticClass()));

    _imageDecoratorOnScreen->EventHandler.BindUObject(
        this,
        &UScreenCreditsWidget::OnPopupClicked);
    _imageDecoratorOnScreen->ScreenBase = this;
    _imageDecoratorOnScreen->_shrinkImageSize = true;
  }
  if (RichTextPopup) {
    RichTextPopup->SetDefaultFont(Font);
    RichTextPopup->SetDefaultColorAndOpacity(
        FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f)));
    _imageDecoratorPopup = static_cast<UCreditsDecorator*>(
        RichTextPopup->GetDecoratorByClass(UCreditsDecorator::StaticClass()));

    _imageDecoratorPopup->ScreenBase = this;
    _imageDecoratorPopup->_shrinkImageSize = false;
  }
  UWorld* world = GetWorld();
  if (world) {
    _pCreditSystem = ACesiumCreditSystem::GetDefaultCreditSystem(GetWorld())
                         ->GetExternalCreditSystem();
  }
}

void UScreenCreditsWidget::HandleImageRequest(
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
    _textureResources[id] = new FSlateDynamicImageBrush(
        Texture,
        FVector2D(Texture->PlatformData->SizeX, Texture->PlatformData->SizeY),
        FName(HttpRequest->GetURL()));
    Invalidate(EInvalidateWidgetReason::Layout);
    _output += TEXT('\u200B');
    if (RichTextOnScreen) {
      RichTextOnScreen->SetText(FText::FromString(_output));
    }
    return;
  }
}

std::string UScreenCreditsWidget::LoadImage(const std::string& url) {

  const std::string base_64_prefix = "data:image/png;base64,";

  if (url.rfind(base_64_prefix, 0) == 0) {
    TArray<uint8> data_buffer;
    FString base64 = UTF8_TO_TCHAR(url.c_str() + base_64_prefix.length());
    if (FBase64::Decode(base64, data_buffer)) {
      UTexture2D* Texture = FImageUtils::ImportBufferAsTexture2D(data_buffer);
      Texture->SRGB = true;
      Texture->UpdateResource();
      Texture->AddToRoot();
      _textureResources.Add(new FSlateDynamicImageBrush(
          Texture,
          FVector2D(Texture->PlatformData->SizeX, Texture->PlatformData->SizeY),
          "Untitled"));
    }
  } else {
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest =
        FHttpModule::Get().CreateRequest();

    _textureResources.Add(nullptr);
    HttpRequest->OnProcessRequestComplete().BindUObject(
        this,
        &UScreenCreditsWidget::HandleImageRequest,
        _textureResources.Num() - 1);

    HttpRequest->SetURL(UTF8_TO_TCHAR(url.c_str()));
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->ProcessRequest();
  }
  return std::to_string(_textureResources.Num() - 1);
}

namespace {
void ConvertHTMLToRTF(
    std::string& output,
    std::string& parentUrl,
    TidyDoc tdoc,
    TidyNode tnod,
    UScreenCreditsWidget* base) {
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
              "<credits url=\"" + parentUrl + "\"" + " text=\"" + text + "\"/>";
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
          output += "<credits id=\"" +
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

UScreenCreditsWidget::~UScreenCreditsWidget() {
  for (int i = 0; i < _textureResources.Num(); i++) {
    if (_textureResources[i]) {
      _textureResources[i]->ReleaseResource();
    }
  }
}

FString UScreenCreditsWidget::ConvertCreditToRTF(
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

void UScreenCreditsWidget::Update() {
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

    bool first = true;
    for (int i = 0; i < creditsToShowThisFrame.size(); i++) {
      const Credit* credit = &creditsToShowThisFrame[i];
      if (!_pCreditSystem->shouldBeShownOnScreen(*credit)) {
        continue;
      }
      if (first) {
        first = false;
      } else {
        _output += TEXT(" \u2022 ");
      }
      if (_creditToRTF.Contains(credit)) {
        _output += _creditToRTF[credit];
      } else {
        FString convert = ConvertCreditToRTF(credit);
        _creditToRTF.Add(credit, convert);
        _output += convert;
      }
    }
    if (RichTextOnScreen) {
      _output += "<credits url=\"popup\" text=\" Data attribution\"/>";
      RichTextOnScreen->SetText(FText::FromString(_output));
    }
  }
  _pCreditSystem->startNextFrame();
}
