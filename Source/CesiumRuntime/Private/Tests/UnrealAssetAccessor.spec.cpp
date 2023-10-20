#include "UnrealAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumRuntime.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"

BEGIN_DEFINE_SPEC(
    FUnrealAssetAccessorSpec,
    "Cesium.Unit.UnrealAssetAccessor",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FUnrealAssetAccessorSpec)

void FUnrealAssetAccessorSpec::Define() {
  It("Can access file:/// URLs", [this]() {
    const char text[] = "Some random text.";

    FString Filename = FPaths::ConvertRelativePathToFull(
        FPaths::CreateTempFilename(*FPaths::ProjectSavedDir()));

    IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
    FFileHelper::SaveStringToFile(
        UTF8_TO_TCHAR(text),
        *Filename,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    try {
      FString Uri = TEXT("file:///") + Filename;
      Uri.ReplaceCharInline('\\', '/');
      Uri.ReplaceInline(TEXT(" "), TEXT("%20"));

      bool done = false;

      UnrealAssetAccessor accessor{};
      accessor.get(getAsyncSystem(), TCHAR_TO_UTF8(*Uri), {})
          .thenInMainThread(
              [&](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                const CesiumAsync::IAssetResponse* Response =
                    pRequest->response();
                TestNotNull("Response", Response);
                if (!Response)
                  return;

                gsl::span<const std::byte> data = Response->data();
                TestEqual("data length", data.size(), sizeof(text) - 1);
                std::string s(
                    reinterpret_cast<const char*>(data.data()),
                    data.size());
                TestEqual("data", s, std::string(text));
                done = true;
              });

      while (!done) {
        accessor.tick();
        getAsyncSystem().dispatchMainThreadTasks();
      }

      FileManager.DeleteFile(*Filename);
    } catch (...) {
      FileManager.DeleteFile(*Filename);
      throw;
    }
  });
}
