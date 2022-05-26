// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/RichTextBlockDecorator.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/ITextDecorator.h"
#include "Framework/Text/TextLayout.h"
#include "Styling/SlateTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include <string>
#include "MyRichTextBlockDecorator.generated.h"

class ISlateStyle;
struct FRichImageRow;

/**
 *
 */
UCLASS()
class UMyRichTextBlockDecorator : public URichTextBlockDecorator {
  GENERATED_BODY()

public:
  UMyRichTextBlockDecorator(const FObjectInitializer& ObjectInitializer);

  void LoadImage(const std::string& url);

  virtual TSharedPtr<ITextDecorator>
  CreateDecorator(URichTextBlock* InOwner) override;

  virtual const FSlateBrush* FindImageBrush(FName TagOrId, bool bWarnIfMissing);

protected:
  FRichImageRow* FindImageRow(FName TagOrId, bool bWarnIfMissing);

  UFUNCTION()
  void OnImageSuccess(UTexture2DDynamic* Texture);

  UFUNCTION()
  void OnImageFailure(UTexture2DDynamic* Texture);

  UPROPERTY(
      EditAnywhere,
      Category = Appearance,
      meta = (RequiredAssetDataTags = "RowStructure=RichImageRow"))
  class UDataTable* ImageSet;
};
