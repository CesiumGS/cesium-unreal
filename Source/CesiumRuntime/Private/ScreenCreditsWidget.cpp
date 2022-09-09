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

class SCreditImage : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SCreditImage) {}
  SLATE_END_ARGS()

public:
  void Construct(const FArguments& InArgs, const FSlateBrush* Brush) {
    if (ensure(Brush)) {
      ChildSlot[SNew(SBox).VAlign(VAlign_Center)[SNew(SImage).Image(Brush)]];
    }
  }
};

class SCreditHyperlinkImage : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SCreditImage) {}
  SLATE_END_ARGS()

public:
  void Construct(
      const FArguments& InArgs,
      const FSlateBrush* Brush,
      const FString& Url) {
    if (ensure(Brush)) {
      ButtonStyle.SetNormal(*Brush);
      ButtonStyle.SetHovered(*Brush);
      ButtonStyle.SetPressed(*Brush);

      ChildSlot
          [SNew(SButton).ButtonStyle(&ButtonStyle).OnClicked_Lambda([Url]() {
            FPlatformProcess::LaunchURL(*Url, NULL, NULL);
            return FReply::Handled();
          })];

      this->SetCursor(EMouseCursor::Hand);
    }
  }

private:
  FButtonStyle ButtonStyle;
};

class SCreditHyperlink : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SCreditImage) {}
  SLATE_END_ARGS()

public:
  void Construct(
      const FArguments& InArgs,
      const FString& Text,
      const FString& Url,
      const UCreditsDecorator* InDecorator) {

    TSharedPtr<FSlateHyperlinkRun::FWidgetViewModel> model =
        MakeShareable(new FSlateHyperlinkRun::FWidgetViewModel);

    ChildSlot[SNew(SRichTextHyperlink, model.ToSharedRef())
                  .Text(FText::FromString(Text))
                  .OnNavigate_Lambda([Url, InDecorator]() {
                    if (Url.Equals("popup")) {
                      InDecorator->PopupClicked.Execute();
                    } else {
                      FPlatformProcess::LaunchURL(*Url, NULL, NULL);
                    }
                  })];
  }
};

class FScreenCreditsDecorator : public FRichTextDecorator {
public:
  FScreenCreditsDecorator(
      URichTextBlock* InOwner,
      UCreditsDecorator* InDecorator)
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
      const FTextBlockStyle&) const override {
    FString Text;
    FString Url;
    const FSlateBrush* Brush = nullptr;
    if (RunInfo.MetaData.Contains(TEXT("url"))) {
      Url = *RunInfo.MetaData[TEXT("url")];
    }
    if (RunInfo.MetaData.Contains(TEXT("text"))) {
      Text = *RunInfo.MetaData[TEXT("text")];
    }
    if (RunInfo.MetaData.Contains(TEXT("id"))) {
      int32 id = FCString::Atoi(*RunInfo.MetaData[TEXT("id")]);
      Brush = Decorator->FindImageBrush(id);
    }
    if (Brush) {
      if (Url.IsEmpty()) {
        return SNew(SCreditImage, Brush);
      } else {
        return SNew(SCreditHyperlinkImage, Brush, Url);
      }
    } else
      return SNew(SCreditHyperlink, Text, Url, Decorator);
  }

private:
  UCreditsDecorator* Decorator;
};

UCreditsDecorator::UCreditsDecorator(
    const FObjectInitializer& ObjectInitializer)
    : URichTextBlockDecorator(ObjectInitializer) {}

TSharedPtr<ITextDecorator>
UCreditsDecorator::CreateDecorator(URichTextBlock* InOwner) {
  return MakeShareable(new FScreenCreditsDecorator(InOwner, this));
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

UScreenCreditsWidget::~UScreenCreditsWidget() {
  for (int i = 0; i < _creditImages.Num(); i++) {
    if (_creditImages[i]) {
      delete _creditImages[i];
    }
  }
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
    _textures.Add(texture);
#if ENGINE_MAJOR_VERSION >= 5
    FTexturePlatformData* pPlatformData = texture->GetPlatformData();
    int32 SizeX = pPlatformData->SizeX;
    int32 SizeY = pPlatformData->SizeY;
#else
    int32 SizeX = texture->PlatformData->SizeX;
    int32 SizeY = texture->PlatformData->SizeY;
#endif
    _creditImages[id] = new FSlateImageBrush(texture, FVector2D(SizeX, SizeY));
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
      _textures.Add(texture);
#if ENGINE_MAJOR_VERSION >= 5
      FTexturePlatformData* pPlatformData = texture->GetPlatformData();
      int32 SizeX = pPlatformData->SizeX;
      int32 SizeY = pPlatformData->SizeY;
#else
      int32 SizeX = texture->PlatformData->SizeX;
      int32 SizeY = texture->PlatformData->SizeY;
#endif
      _creditImages.Add(new FSlateImageBrush(texture, FVector2D(SizeX, SizeY)));
    }
  } else {
    ++_numImagesLoading;
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest =
        FHttpModule::Get().CreateRequest();

    _creditImages.AddDefaulted();
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
