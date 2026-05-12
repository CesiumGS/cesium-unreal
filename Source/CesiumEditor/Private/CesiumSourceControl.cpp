// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumSourceControl.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformFileManager.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "Misc/MessageDialog.h"
#include "SourceControlOperations.h"
#include "Widgets/Notifications/SNotificationList.h"

void CesiumSourceControl::PromptToCheckoutConfigFile(
    const FString& RelativeConfigFilePath) {
  if (ISourceControlModule::Get().IsEnabled()) {
    FString ConfigFilePath =
        FPaths::ConvertRelativePathToFull(RelativeConfigFilePath);
    FText ConfigFilename =
        FText::FromString(FPaths::GetCleanFilename(ConfigFilePath));

    ISourceControlProvider& SourceControlProvider =
        ISourceControlModule::Get().GetProvider();
    FSourceControlStatePtr SourceControlState =
        SourceControlProvider.GetState(ConfigFilePath, EStateCacheUsage::Use);

    if (SourceControlState.IsValid() &&
        SourceControlState->IsSourceControlled()) {

      TArray<FString> FilesToBeCheckedOut;
      FilesToBeCheckedOut.Add(ConfigFilePath);
      if (SourceControlState->CanCheckout() ||
          SourceControlState->IsCheckedOutOther() ||
          FPlatformFileManager::Get().GetPlatformFile().IsReadOnly(
              *ConfigFilePath)) {
        FString Message = FString::Format(
            TEXT(
                "The default access token is saved in {0} which is currently not checked out. Would you like to check it out from source control?"),
            {ConfigFilename.ToString()});

        if (FMessageDialog::Open(
                EAppMsgType::YesNo,
                FText::FromString(Message)) == EAppReturnType::Yes) {
          ECommandResult::Type CommandResult = SourceControlProvider.Execute(
              ISourceControlOperation::Create<FCheckOut>(),
              FilesToBeCheckedOut);

          if (CommandResult != ECommandResult::Succeeded) {
            // Show a notification that the file could not be checked out
            FNotificationInfo CheckOutError(FText::FromString(
                TEXT("Error: Failed to check out the configuration file.")));
            CheckOutError.ExpireDuration = 3.0f;
            FSlateNotificationManager::Get().AddNotification(CheckOutError);
          }
        }
      }
    }
  }
}
