#include "EncryptionUtility.h"
#include "CryptoPP/files.h"
#include "CryptoPP/base64.h"
#include "CryptoPP/cryptlib.h"
#include "CryptoPP/filters.h"
#include "CryptoPP/modes.h"
#include "CryptoPP/rsa.h"


CryptoPP::AutoSeededRandomPool& UEncryptionUtility::getRng() {
  static CryptoPP::AutoSeededRandomPool m_rng;
  return m_rng;
}

FString UEncryptionUtility::GetAESKeyByFile(FString Path) {
  std::string res;
  try {
    CryptoPP::FileSource KeyFile(TCHAR_TO_ANSI(*Path),true,
      new CryptoPP::StringSink(res));
  }
  catch(CryptoPP::Exception &e) {
    res=" ";
    UE_LOG(LogTemp,Warning,TEXT("SET AESKEY FAILED!ERROR:%hs"),e.what());
    return UTF8_TO_TCHAR(res.c_str());
  }
  return UTF8_TO_TCHAR(res.c_str());
}

FString UEncryptionUtility::RSAEncryptData(FString Input,FString KeyPath) {
  std::string result;
  CryptoPP::RSA::PublicKey PubRSAKey;
  CryptoPP::ByteQueue queue;
  try {
    CryptoPP::FileSource privFile(TCHAR_TO_ANSI(*KeyPath),true,new CryptoPP::Base64Decoder);
    privFile.TransferTo(queue);
    queue.MessageEnd();
    PubRSAKey.BERDecodePublicKey(queue,false,queue.MaxRetrievable());
  }
  catch(CryptoPP::Exception &e) {
    UE_LOG(LogTemp,Warning,TEXT("GET OR SET PUBLIC RSAKEY FAILED!ERROR:%hs"),e.what());
  }
  try {
    CryptoPP::RSAES_PKCS1v15_Encryptor d(PubRSAKey);
    CryptoPP::StringSource(TCHAR_TO_UTF8(*Input),true,
      new CryptoPP::Base64Decoder(
        new CryptoPP::PK_EncryptorFilter(getRng(),d,
          new CryptoPP::StringSink(result))));
  }
  catch(CryptoPP::Exception &e) {
    result="";
    UE_LOG(LogTemp,Warning,TEXT("RSAENCRYPTDATA FAILED!ERROR:%hs"),e.what());
  }
  return UTF8_TO_TCHAR(result.c_str());
}

FString UEncryptionUtility::RSADecryptData(FString Input,FString KeyPath) {
  std::string result;
  CryptoPP::RSA::PrivateKey PrivRSAKey;
  CryptoPP::ByteQueue queue;
  try {
    CryptoPP::FileSource privFile(TCHAR_TO_ANSI(*KeyPath),true,
      new CryptoPP::Base64Decoder);
    privFile.TransferTo(queue);
    queue.MessageEnd();
    PrivRSAKey.BERDecodePrivateKey(queue,false,queue.MaxRetrievable());
  }
  catch(CryptoPP::Exception &e) {
    UE_LOG(LogTemp,Warning,TEXT("GET OR SET PRIVATE RSAKEY FAILED!ERROR:%hs"),e.what());
  }
  try {
    CryptoPP::RSAES_PKCS1v15_Decryptor d(PrivRSAKey);
    CryptoPP::StringSource(TCHAR_TO_UTF8(*Input),true,
      new CryptoPP::Base64Decoder(
        new CryptoPP::PK_DecryptorFilter(getRng(),d,
          new CryptoPP::StringSink(result))));
  }
  catch(CryptoPP::Exception &e) {
    result="";
    UE_LOG(LogTemp,Warning,TEXT("RSADECRYPTDATA FAILED!ERROR:%hs"),e.what());
  }
  return UTF8_TO_TCHAR(result.c_str());
}

FString UEncryptionUtility::CBC_AESDecryptData(FString Input,FString Key,FString IV) {
  const std::string CipherText=TCHAR_TO_UTF8(*Input);
  const std::string SAESKey=TCHAR_TO_UTF8(*Key);
  const std::string SIV=TCHAR_TO_UTF8(*IV);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  memset(key,0x20,key.size());//0x20 =' ',0x30='0'
  SAESKey.size()<=CryptoPP::AES::MAX_KEYLENGTH
  ?memcpy(key,SAESKey.c_str(),SAESKey.size())
  :memcpy(key,SAESKey.c_str(),CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  memset(iv,0x20,CryptoPP::AES::BLOCKSIZE);
  SIV.size() <=CryptoPP::AES::BLOCKSIZE
  ?memcpy(iv,SIV.c_str(),SIV.size())
  :memcpy(iv,SIV.c_str(),CryptoPP::AES::BLOCKSIZE);
  std::string Base64decodeCipher;
  std::string Plain;
  try {
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption cbcDecryption;
    cbcDecryption.SetKeyWithIV((CryptoPP::byte*)key,CryptoPP::AES::MAX_KEYLENGTH,iv);
    CryptoPP::StringSource(CipherText,true,
      new CryptoPP::Base64Decoder(
        new CryptoPP::StringSink(Base64decodeCipher)));
    CryptoPP::StringSource(Base64decodeCipher,true,
      new CryptoPP::StreamTransformationFilter(cbcDecryption,
        new CryptoPP::StringSink(Plain),CryptoPP::BlockPaddingSchemeDef::NO_PADDING));
  }
  catch(CryptoPP::Exception &e) {
    Plain="";
    UE_LOG(LogTemp,Warning,TEXT("CBC_AESDecryption failed!Error:%hs"),e.what());
  }
  return UTF8_TO_TCHAR(Plain.c_str());
}

FString UEncryptionUtility::CBC_AESEncryptData(FString Input,FString Key,FString IV) {
  const std::string PlainText=TCHAR_TO_UTF8(*Input);
  const std::string SAESKey=TCHAR_TO_UTF8(*Key);
  const std::string SIV=TCHAR_TO_UTF8(*IV);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  memset(key,0x20,key.size());//0x20 =' ',0x30='0'
  SAESKey.size()<=CryptoPP::AES::MAX_KEYLENGTH
  ?memcpy(key,SAESKey.c_str(),SAESKey.size())
  :memcpy(key,SAESKey.c_str(),CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  memset(iv,0x20,CryptoPP::AES::BLOCKSIZE);
  SIV.size() <=CryptoPP::AES::BLOCKSIZE
  ?memcpy(iv,SIV.c_str(),SIV.size())
  :memcpy(iv,SIV.c_str(),CryptoPP::AES::BLOCKSIZE);
  std::string Cipher;
  std::string Base64EncodeCipher;
  try {
    CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption cbcEncryption;
    cbcEncryption.SetKeyWithIV((CryptoPP::byte*)key,CryptoPP::AES::MAX_KEYLENGTH,iv);
    CryptoPP::StringSource(PlainText,true,
      new CryptoPP::StreamTransformationFilter(cbcEncryption,
        new CryptoPP::StringSink(Cipher),CryptoPP::BlockPaddingSchemeDef::ZEROS_PADDING));
    CryptoPP::StringSource(Cipher,true,
      new CryptoPP::Base64Encoder(
        new CryptoPP::StringSink(Base64EncodeCipher)));
  }
  catch(CryptoPP::Exception &e) {
    Cipher="";
    UE_LOG(LogTemp,Warning,TEXT("CBC_AESDecryption failed!Error:%hs"),e.what());
  }
  return UTF8_TO_TCHAR(Base64EncodeCipher.c_str());
}

FString UEncryptionUtility::ECB_AESDecryptData(FString Input,FString Key) {
  const std::string CipherText=TCHAR_TO_UTF8(*Input);
  const std::string SAESKey=TCHAR_TO_UTF8(*Key);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  memset(key,0x20,key.size());//0x20 =' ',0x30='0'
  SAESKey.size()<=CryptoPP::AES::MAX_KEYLENGTH
  ?memcpy(key,SAESKey.c_str(),SAESKey.size())
  :memcpy(key,SAESKey.c_str(),CryptoPP::AES::MAX_KEYLENGTH);
  std::string Base64decodeCipher;
  std::string Plain;
  try {
    CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption aesDecryption;
    aesDecryption.SetKey((CryptoPP::byte*)key,CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::StringSource(CipherText,true,
      new CryptoPP::Base64Decoder(
        new CryptoPP::StringSink(Base64decodeCipher)));
    CryptoPP::StringSource(Base64decodeCipher,true,
      new CryptoPP::StreamTransformationFilter(aesDecryption,
        new CryptoPP::StringSink(Plain),CryptoPP::BlockPaddingSchemeDef::NO_PADDING));
  }
  catch(CryptoPP::Exception &e) {
    Plain="";
    UE_LOG(LogTemp, Warning,TEXT("ECB_AESDecryption failed!Error:%hs"),e.what());
  }
  return UTF8_TO_TCHAR(Plain.c_str());
}

FString UEncryptionUtility::ECB_AESEncryptData(FString Input,FString Key) {
  const std::string PlainText=TCHAR_TO_UTF8(*Input);
  const std::string SAESKey=TCHAR_TO_UTF8(*Key);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  memset(key,0x20,key.size());//0x20 =' ',0x30='0'
  SAESKey.size()<=CryptoPP::AES::MAX_KEYLENGTH
  ?memcpy(key,SAESKey.c_str(),SAESKey.size())
  :memcpy(key,SAESKey.c_str(),CryptoPP::AES::MAX_KEYLENGTH);
  std::string Cipher;
  std::string Base64EncodeCipher;
  try {
    CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption aesEncryption;
    aesEncryption.SetKey((CryptoPP::byte*)key,CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::StringSource(PlainText,true,
      new CryptoPP::StreamTransformationFilter(aesEncryption,
        new CryptoPP::StringSink(Cipher),CryptoPP::BlockPaddingSchemeDef::ZEROS_PADDING));
    CryptoPP::StringSource(Cipher,true,
      new CryptoPP::Base64Encoder(
        new CryptoPP::StringSink(Base64EncodeCipher)));
  }
  catch(CryptoPP::Exception &e) {
    Base64EncodeCipher="";
    UE_LOG(LogTemp, Warning,TEXT("ECB_AESEncryption failed!Error:%hs"),e.what());
  }
  return UTF8_TO_TCHAR(Base64EncodeCipher.c_str());
}

std::string UEncryptionUtility::S_RSADecryptData(gsl::span<const std::byte> Input, FString KeyPath) {
  std::string Plain;
  CryptoPP::RSA::PrivateKey PrivRSAKey;
  CryptoPP::ByteQueue queue;
  try {
    CryptoPP::FileSource privFile(TCHAR_TO_ANSI(*KeyPath),true,
      new CryptoPP::Base64Decoder);
    privFile.TransferTo(queue);
    queue.MessageEnd();
    PrivRSAKey.BERDecodePrivateKey(queue,false,queue.MaxRetrievable());
  }
  catch(CryptoPP::Exception &e) {
    UE_LOG(LogTemp,Warning,TEXT("GET OR SET PRIVATE RSAKEY FAILED!ERROR:%hs"),e.what());
  }
  std::string StrInput(reinterpret_cast<const char*>(Input.data()),Input.size());
  try {
    CryptoPP::RSAES_PKCS1v15_Decryptor d(PrivRSAKey);
    CryptoPP::StringSource(StrInput,true,
      new CryptoPP::Base64Decoder(
        new CryptoPP::PK_DecryptorFilter(getRng(),d,
          new CryptoPP::StringSink(Plain))));
  }
  catch(CryptoPP::Exception &e) {
    Plain="";
    UE_LOG(LogTemp,Warning,TEXT("RSADECRYPTDATA FAILED!ERROR:%hs"),e.what());
  }
  return Plain;
}

std::string UEncryptionUtility::S_ECB_AESDecryptData(gsl::span<const std::byte> Input, FString Key) {
  const std::string CipherText(reinterpret_cast<const char*>(Input.data()),Input.size());
  const std::string SAESKey=TCHAR_TO_UTF8(*Key);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  memset(key,0x20,key.size());//0x20 =' ',0x30='0'
  SAESKey.size()<=CryptoPP::AES::MAX_KEYLENGTH
  ?memcpy(key,SAESKey.c_str(),SAESKey.size())
  :memcpy(key,SAESKey.c_str(),CryptoPP::AES::MAX_KEYLENGTH);
  //std::string Base64decodeCipher;
  std::string Plain;
  try {
    CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption aesDecryption;
    aesDecryption.SetKey((CryptoPP::byte*)key,CryptoPP::AES::MAX_KEYLENGTH);
    /*CryptoPP::StringSource(CipherText,true,
      new CryptoPP::Base64Decoder(
        new CryptoPP::StringSink(Base64decodeCipher)));*/
    //Use Base64 Encode or not?🤔
    CryptoPP::StringSource(CipherText,true,
      new CryptoPP::StreamTransformationFilter(aesDecryption,
        new CryptoPP::StringSink(Plain),CryptoPP::BlockPaddingSchemeDef::NO_PADDING));
  }
  catch(CryptoPP::Exception &e) {
    Plain="";
    UE_LOG(LogTemp, Warning,TEXT("ECB_AESDecryption failed!Error:%hs"),e.what());
  }
  return Plain;
}

std::string UEncryptionUtility::S_CBC_AESDecryptData(gsl::span<const std::byte> Input, FString Key, FString IV) {
  const std::string CipherText(reinterpret_cast<const char*>(Input.data()),Input.size());
  const std::string SAESKey=TCHAR_TO_UTF8(*Key);
  const std::string SIV=TCHAR_TO_UTF8(*IV);
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  memset(key,0x20,key.size());//0x20 =' ',0x30='0'
  SAESKey.size()<=CryptoPP::AES::MAX_KEYLENGTH
  ?memcpy(key,SAESKey.c_str(),SAESKey.size())
  :memcpy(key,SAESKey.c_str(),CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  memset(iv,0x20,CryptoPP::AES::BLOCKSIZE);
  SIV.size() <=CryptoPP::AES::BLOCKSIZE
  ?memcpy(iv,SIV.c_str(),SIV.size())
  :memcpy(iv,SIV.c_str(),CryptoPP::AES::BLOCKSIZE);
  std::string Base64decodeCipher;
  std::string Plain;
  try {
    CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption cbcDecryption;
    cbcDecryption.SetKeyWithIV((CryptoPP::byte*)key,CryptoPP::AES::MAX_KEYLENGTH,iv);
    CryptoPP::StringSource(CipherText,true,
      new CryptoPP::Base64Decoder(
        new CryptoPP::StringSink(Base64decodeCipher)));
    CryptoPP::StringSource(Base64decodeCipher,true,
      new CryptoPP::StreamTransformationFilter(cbcDecryption,
        new CryptoPP::StringSink(Plain),CryptoPP::BlockPaddingSchemeDef::NO_PADDING));
  }
  catch(CryptoPP::Exception &e) {
    Plain="";
    UE_LOG(LogTemp,Warning,TEXT("CBC_AESDecryption failed!Error:%hs"),e.what());
  }
  return Plain;
}
