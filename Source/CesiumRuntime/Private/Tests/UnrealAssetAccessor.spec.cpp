// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "UnrealAssetAccessor.h"
#include "Async/Async.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumRuntime.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

BEGIN_DEFINE_SPEC(
    FUnrealAssetAccessorSpec,
    "Cesium.Unit.UnrealAssetAccessor",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

FString Filename;
std::string randomText = "Some random text.";
IPlatformFile* FileManager;

void TestAccessorRequest(const FString& Uri, const std::string& expectedData) {
  bool done = false;

  UnrealAssetAccessor accessor{};
  accessor.get(getAsyncSystem(), TCHAR_TO_UTF8(*Uri), {})
      .thenInMainThread(
          [&](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
            const CesiumAsync::IAssetResponse* Response = pRequest->response();
            TestNotNull("Response", Response);
            if (!Response)
              return;

            gsl::span<const std::byte> data = Response->data();
            TestEqual("data length", data.size(), expectedData.size());
            std::string s(
                reinterpret_cast<const char*>(data.data()),
                data.size());
            TestEqual("data", s, expectedData);
            done = true;
          });

  while (!done) {
    accessor.tick();
    getAsyncSystem().dispatchMainThreadTasks();
  }
}

END_DEFINE_SPEC(FUnrealAssetAccessorSpec)

void FUnrealAssetAccessorSpec::Define() {
  BeforeEach([this]() {
    Filename = FPaths::ConvertRelativePathToFull(
        FPaths::CreateTempFilename(*FPaths::ProjectSavedDir()));

    FileManager = &FPlatformFileManager::Get().GetPlatformFile();
    FFileHelper::SaveStringToFile(
        UTF8_TO_TCHAR(randomText.c_str()),
        *Filename,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
  });

  AfterEach([this]() { FileManager->DeleteFile(*Filename); });

  It("Fails with non-existant file:/// URLs", [this]() {
    FString Uri = TEXT("file:///") + Filename;
    Uri.ReplaceCharInline('\\', '/');
    Uri.ReplaceInline(TEXT(" "), TEXT("%20"));
    Uri += ".bogusExtension";

    TestAccessorRequest(Uri, "");
  });

  It("Can access file:/// URLs", [this]() {
    FString Uri = TEXT("file:///") + Filename;
    Uri.ReplaceCharInline('\\', '/');
    Uri.ReplaceInline(TEXT(" "), TEXT("%20"));

    TestAccessorRequest(Uri, randomText);
  });

  It("Can access file:/// URLs with unnecessary query params", [this]() {
    FString Uri = TEXT("file:///") + Filename;
    Uri.ReplaceCharInline('\\', '/');
    Uri.ReplaceInline(TEXT(" "), TEXT("%20"));
    Uri.Append("?version=4.27.1");

    TestAccessorRequest(Uri, randomText);
  });
}
