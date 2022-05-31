// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "MyRichTextBlockDecorator.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Math/UnrealMathUtility.h"
#include "Rendering/DrawElements.h"
#include "Slate/SlateGameResources.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SRichTextHyperlink.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SCompoundWidget.h"
#include <string>

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
      const FTextBlockStyle& TextStyle) {
    if (ensure(Brush)) {
      const TSharedRef<FSlateFontMeasure> FontMeasure =
          FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
      float IconHeight = FMath::Min(
          (float)FontMeasure->GetMaxCharacterHeight(TextStyle.Font, 1.5f),
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
                    .OnNavigate_Lambda([url]() {
                      FPlatformProcess::LaunchURL(*url, NULL, NULL);
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
    const FSlateBrush* Brush;
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
    return SNew(SRichInlineImage, Brush, url, text, TextStyle);
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
