// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "MyRichTextBlockDecorator.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Math/UnrealMathUtility.h"
#include "Rendering/DrawElements.h"
#include "Slate/SlateGameResources.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SCompoundWidget.h"

class SRichInlineImage : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SRichInlineImage) {}
  SLATE_END_ARGS()

public:
  void Construct(
      const FArguments& InArgs,
      const FSlateBrush* Brush,
      const FTextBlockStyle& TextStyle) {
    if (ensure(Brush)) {
      const TSharedRef<FSlateFontMeasure> FontMeasure =
          FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
      float IconHeight = FMath::Min(
          (float)FontMeasure->GetMaxCharacterHeight(TextStyle.Font, 1.5f),
          Brush->ImageSize.Y);
      float IconWidth =
          IconHeight / (float)Brush->ImageSize.Y * Brush->ImageSize.X;
      ChildSlot[SNew(SBox)
                    .HeightOverride(IconHeight)
                    .WidthOverride(IconWidth)
                    .VAlign(VAlign_Center)[SNew(SImage).Image(Brush)]];
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
        RunParseResult.MetaData.Contains(TEXT("id"))) {
      return true;
    }
    return false;
  }

protected:
  virtual TSharedPtr<SWidget> CreateDecoratorWidget(
      const FTextRunInfo& RunInfo,
      const FTextBlockStyle& TextStyle) const override {

    int32 id = FCString::Atoi(*RunInfo.MetaData[TEXT("id")]);
    const FSlateBrush* Brush = Decorator->FindImageBrush(id);

    if (Brush) {
      return SNew(SRichInlineImage, Brush, TextStyle);
    }
    return TSharedPtr<SWidget>();
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
