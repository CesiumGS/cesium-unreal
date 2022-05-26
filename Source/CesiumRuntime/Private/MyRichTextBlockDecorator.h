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

  virtual TSharedPtr<ITextDecorator>
  CreateDecorator(URichTextBlock* InOwner) override;

  virtual const FSlateBrush* FindImageBrush(FName TagOrId, bool bWarnIfMissing);

protected:
  FRichImageRow* FindImageRow(FName TagOrId, bool bWarnIfMissing);

  UPROPERTY(
      EditAnywhere,
      Category = Appearance,
      meta = (RequiredAssetDataTags = "RowStructure=RichImageRow"))
  class UDataTable* ImageSet;
};
