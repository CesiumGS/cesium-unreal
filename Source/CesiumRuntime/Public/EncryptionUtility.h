#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CryptoPP/osrng.h"
#include <gsl/span>
#include <gsl/util>

#include "EncryptionUtility.generated.h"

UCLASS()
class CESIUMRUNTIME_API UEncryptionUtility:public UBlueprintFunctionLibrary{
public:
  GENERATED_BODY()
  UFUNCTION(BlueprintCallable, Category = "EncryptionUtility")
  static FString ECB_AESEncryptData(FString Input,FString Key);
  UFUNCTION(BlueprintCallable, Category = "EncryptionUtility")
  static FString ECB_AESDecryptData(FString Input,FString Key);
  UFUNCTION(BlueprintCallable, Category = "EncryptionUtility")
  static FString CBC_AESEncryptData(FString Input,FString Key,FString IV);
  UFUNCTION(BlueprintCallable, Category = "EncryptionUtility")
  static FString CBC_AESDecryptData(FString Input,FString Key,FString IV);
  UFUNCTION(BlueprintCallable, Category = "EncryptionUtility")
  static FString RSAEncryptData(FString Input,FString KeyPath);
  UFUNCTION(BlueprintCallable, Category = "EncryptionUtility")
  static FString RSADecryptData(FString Input,FString KeyPath);
  UFUNCTION(BlueprintCallable, Category = "EncryptionUtility")
  static FString GetAESKeyByFile(FString Path);

  static std::string S_RSADecryptData(gsl::span<const std::byte> Input,FString KeyPath);
  static std::string S_ECB_AESDecryptData(gsl::span<const std::byte> Input,FString Key);
  static std::string S_CBC_AESDecryptData(gsl::span<const std::byte> Input,FString Key,FString IV);
private:
  static CryptoPP::AutoSeededRandomPool& getRng();

};

