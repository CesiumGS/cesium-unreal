// Fill out your copyright notice in the Description page of Project Settings.

#include "MyScreenCreditsBase.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Components/RichTextBlock.h"
#include <tidybuffio.h>
#include <vector>

using namespace Cesium3DTilesSelection;

UMyScreenCreditsBase::UMyScreenCreditsBase(
    const FObjectInitializer& ObjectInitializer)
    : UUserWidget(ObjectInitializer) {}

void UMyScreenCreditsBase::Update() {
  if (!_pCreditSystem) {
    UWorld* world = GetWorld();
    if (world) {
      _pCreditSystem = ACesiumCreditSystem::GetDefaultCreditSystem(GetWorld())
                           ->GetExternalCreditSystem();
    }
  }

  const std::vector<Cesium3DTilesSelection::Credit>& creditsToShowThisFrame =
      _pCreditSystem->getCreditsToShowThisFrame();

  bool CreditsUpdated =
      creditsToShowThisFrame.size() != _lastCreditsCount ||
      _pCreditSystem->getCreditsToNoLongerShowThisFrame().size() > 0;

  if (CreditsUpdated) {
    _lastCreditsCount = creditsToShowThisFrame.size();

    FString output;

    for (int i = 0; i < creditsToShowThisFrame.size(); i++) {
      if (i != 0) {
        output += " ";
      }
      const Credit* credit = &creditsToShowThisFrame[i];
      // if (_creditToRTF.Contains(credit)) {
      //   output += _creditToRTF[credit];
      // } else {
      FString convert = ConvertCreditToRTF(credit);
      // _creditToRTF.Add(credit, convert);
      output += convert;
      //}
    }
    if (RichTextBlock_127) {
      RichTextBlock_127->SetText(FText::FromString(output));
    }
  }
  _pCreditSystem->startNextFrame();
}

namespace {
void ConvertHTMLToRTF(std::string& html, TidyDoc tdoc, TidyNode tnod) {
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
    }
    ConvertHTMLToRTF(html, tdoc, child);
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
  tidyOptSetBool(tdoc, TidyForceOutput, yes); /* try harder */
  tidyOptSetInt(tdoc, TidyWrapLen, 4096);
  tidySetErrorBuffer(tdoc, &tidy_errbuf);
  tidyBufInit(&docbuf);

  if (html.find("<") == html.npos) {
    html = "<span>" + html + "</span>";
  }

  tidyBufAppend(
      &docbuf,
      reinterpret_cast<void*>(const_cast<char*>(html.c_str())),
      static_cast<uint>(html.size()));

  err = tidyParseBuffer(tdoc, &docbuf); /* parse the input */

  std::string output;
  ConvertHTMLToRTF(output, tdoc, tidyGetRoot(tdoc));

  tidyBufFree(&docbuf);
  tidyBufFree(&tidy_errbuf);
  tidyRelease(tdoc);

  // remove new line '/r/n'
  if (output.size() > 2) {
    output.pop_back();
    output.pop_back();
  }

  return UTF8_TO_TCHAR(output.c_str());
}
