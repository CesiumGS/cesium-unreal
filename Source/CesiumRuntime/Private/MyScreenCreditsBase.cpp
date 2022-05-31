// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "MyScreenCreditsBase.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "CesiumCreditSystem.h"
#include "Components/RichTextBlock.h"
#include "Engine/Texture2D.h"
#include "HttpModule.h"
#include "ImageUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Base64.h"
#include "MyRichTextBlockDecorator.h"
#include <tidybuffio.h>
#include <vector>

using namespace Cesium3DTilesSelection;

UMyScreenCreditsBase::UMyScreenCreditsBase(
    const FObjectInitializer& ObjectInitializer)
    : UUserWidget(ObjectInitializer) {}

void UMyScreenCreditsBase::NativeConstruct() {
  if (RichTextBlock_127) {
    _imageDecorator = static_cast<UMyRichTextBlockDecorator*>(
        RichTextBlock_127->GetDecoratorByClass(
            UMyRichTextBlockDecorator::StaticClass()));
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
      RichTextBlock_127->SetText(FText::FromString(_output));
    }
  }
  _pCreditSystem->startNextFrame();
}

namespace {
void ConvertHTMLToRTF(
    std::string& html,
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
        html += reinterpret_cast<const char*>(buf.bp);
      }
      tidyBufFree(&buf);
    } else if (tidyNodeGetId(child) == TidyTagId::TidyTag_IMG) {
      auto srcAttr = tidyAttrGetById(child, TidyAttrId::TidyAttr_SRC);
      if (srcAttr) {
        auto srcValue = tidyAttrValue(srcAttr);
        if (srcValue) {
          html += "<img id=\"" +
                  base->LoadImage(
                      std::string(reinterpret_cast<const char*>(srcValue))) +
                  "\"/>";
        }
      }
    }
    ConvertHTMLToRTF(html, tdoc, child, base);
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

  std::string output;
  ConvertHTMLToRTF(output, tdoc, tidyGetRoot(tdoc), this);

  tidyBufFree(&docbuf);
  tidyBufFree(&tidy_errbuf);
  tidyRelease(tdoc);

  // could not find correct option in tidy html to not add new lines
  if (output.size() != 0 && output[output.size() - 1] == '\n') {
    output.pop_back();
  }
  return UTF8_TO_TCHAR(output.c_str());
}
