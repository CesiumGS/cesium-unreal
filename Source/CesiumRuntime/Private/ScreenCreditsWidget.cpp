// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "ScreenCreditsWidget.h"
#include "Components/BackgroundBlur.h"
#include "Components/RichTextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Base64.h"
#include "Rendering/DrawElements.h"
#include "Slate/SlateGameResources.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SRichTextHyperlink.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCompoundWidget.h"
#include <string>
#include <vector>

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
      ChildSlot[SNew(SBox).VAlign(
          VAlign_Center)[SNew(SImage).Image(Brush).OnMouseButtonDown_Lambda(
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
                        InDecorator->PopupClicked.Execute();
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
    return SNew(SRichInlineImage, Brush, url, text, TextStyle, Decorator);
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
  if (CreditsWidget->_creditImages.Num() > id) {
    return CreditsWidget->_creditImages[id];
  }
  return nullptr;
}

UScreenCreditsWidget::UScreenCreditsWidget(
    const FObjectInitializer& ObjectInitializer)
    : UUserWidget(ObjectInitializer) {
  static ConstructorHelpers::FObjectFinder<UFont> RobotoFontObj(
      *UWidget::GetDefaultFontName());
  _font = FSlateFontInfo(RobotoFontObj.Object, 8);
}

void UScreenCreditsWidget::OnPopupClicked() {
  _showPopup = !_showPopup;
  BackgroundBlur->SetVisibility(
      _showPopup ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void UScreenCreditsWidget::NativeConstruct() {

  Super::NativeConstruct();

  if (RichTextOnScreen) {
    RichTextOnScreen->SetDefaultFont(_font);
    RichTextOnScreen->SetDefaultColorAndOpacity(
        FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f)));
    _decoratorOnScreen =
        static_cast<UCreditsDecorator*>(RichTextOnScreen->GetDecoratorByClass(
            UCreditsDecorator::StaticClass()));

    _decoratorOnScreen->PopupClicked.BindUObject(
        this,
        &UScreenCreditsWidget::OnPopupClicked);
    _decoratorOnScreen->CreditsWidget = this;
  }
  if (RichTextPopup) {
    RichTextPopup->SetDefaultFont(_font);
    RichTextPopup->SetDefaultColorAndOpacity(
        FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f)));
    _decoratorPopup = static_cast<UCreditsDecorator*>(
        RichTextPopup->GetDecoratorByClass(UCreditsDecorator::StaticClass()));

    _decoratorPopup->CreditsWidget = this;
  }
}

void UScreenCreditsWidget::HandleImageRequest(
    FHttpRequestPtr HttpRequest,
    FHttpResponsePtr HttpResponse,
    bool bSucceeded,
    int32 id) {
  if (bSucceeded && HttpResponse.IsValid() &&
      HttpResponse->GetContentLength() > 0) {
    UTexture2D* texture =
        FImageUtils::ImportBufferAsTexture2D(HttpResponse->GetContent());
    texture->SRGB = true;
    texture->UpdateResource();
    texture->AddToRoot();
    _creditImages[id] = new FSlateDynamicImageBrush(
        texture,
        FVector2D(texture->PlatformData->SizeX, texture->PlatformData->SizeY),
        FName(HttpRequest->GetURL()));
    // Only update credits after all of the images are done loading.
    --_numImagesLoading;
    if (_numImagesLoading == 0) {
      SetCredits(_credits, _onScreenCredits);
    }
    return;
  } else {
    --_numImagesLoading;
  }
}

std::string UScreenCreditsWidget::LoadImage(const std::string& url) {
  const std::string base64Prefix = "data:image/png;base64,";
  if (url.rfind(base64Prefix, 0) == 0) {
    TArray<uint8> dataBuffer;
    FString base64 = UTF8_TO_TCHAR(url.c_str() + base64Prefix.length());
    if (FBase64::Decode(base64, dataBuffer)) {
      UTexture2D* texture = FImageUtils::ImportBufferAsTexture2D(dataBuffer);
      texture->SRGB = true;
      texture->UpdateResource();
      texture->AddToRoot();
      _creditImages.Add(new FSlateDynamicImageBrush(
          texture,
          FVector2D(texture->PlatformData->SizeX, texture->PlatformData->SizeY),
          "Untitled"));
    }
  } else {
    ++_numImagesLoading;
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest =
        FHttpModule::Get().CreateRequest();

    _creditImages.Add(nullptr);
    HttpRequest->OnProcessRequestComplete().BindUObject(
        this,
        &UScreenCreditsWidget::HandleImageRequest,
        _creditImages.Num() - 1);

    HttpRequest->SetURL(UTF8_TO_TCHAR(url.c_str()));
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->ProcessRequest();
  }
  return std::to_string(_creditImages.Num() - 1);
}

void UScreenCreditsWidget::SetCredits(
    const FString& InCredits,
    const FString& InOnScreenCredits) {
  if (_numImagesLoading != 0) {
    _credits = InCredits;
    _onScreenCredits = InOnScreenCredits;
    return;
  }
  if (RichTextPopup) {
    RichTextPopup->SetText(FText::FromString(InCredits));
  }
  if (RichTextOnScreen) {
    RichTextOnScreen->SetText(FText::FromString(InOnScreenCredits));
  }
}

UScreenCreditsWidget::~UScreenCreditsWidget() {
  for (int i = 0; i < _creditImages.Num(); i++) {
    if (_creditImages[i]) {
      _creditImages[i]->ReleaseResource();
    }
  }
}
