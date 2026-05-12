// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "VecMath.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(
    FVecMathSpec,
    "Cesium.Unit.VecMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FVecMathSpec)

void FVecMathSpec::Define() {
  Describe("createTransform", [this]() {
    It("matches FMatrix -> FTransform for larger scales", [this]() {
      FTransform original = FTransform(
          FQuat::MakeFromRotator(FRotator(10.0, 20.0, 30.0)),
          FVector(3000.0, 2000.0, 1000.0),
          FVector(1.0, 2.0, 3.0));

      FMatrix originalUnrealMatrix = original.ToMatrixWithScale();
      glm::dmat4 originalGlmMatrix =
          VecMath::createMatrix4D(originalUnrealMatrix);

      FTransform viaUnrealMatrix =
          FTransform(VecMath::createMatrix(originalGlmMatrix));
      FTransform viaVecMath = VecMath::createTransform(originalGlmMatrix);

      TestNearlyEqual(
          TEXT("Translation"),
          viaVecMath.GetTranslation(),
          viaUnrealMatrix.GetTranslation(),
          1e-8);
      TestNearlyEqual(
          TEXT("Rotation"),
          viaVecMath.GetRotation().Rotator(),
          viaUnrealMatrix.GetRotation().Rotator(),
          1e-10);
      TestNearlyEqual(
          TEXT("Scale"),
          viaVecMath.GetScale3D(),
          viaUnrealMatrix.GetScale3D(),
          1e-11);
    });

    It("returns correct values when scale is small", [this]() {
      FTransform original = FTransform(
          FQuat::MakeFromRotator(FRotator(10.0, 20.0, 30.0)),
          FVector(3000.0, 2000.0, 1000.0),
          FVector(1e-7, 2e-7, 3e-7));

      FMatrix originalUnrealMatrix = original.ToMatrixWithScale();
      glm::dmat4 originalGlmMatrix =
          VecMath::createMatrix4D(originalUnrealMatrix);

      FTransform viaVecMath = VecMath::createTransform(originalGlmMatrix);

      TestNearlyEqual(
          TEXT("Translation"),
          viaVecMath.GetTranslation(),
          original.GetTranslation(),
          1e-8);
      TestNearlyEqual(
          TEXT("Rotation"),
          viaVecMath.GetRotation().Rotator(),
          original.GetRotation().Rotator(),
          1e-10);
      TestNearlyEqual(
          TEXT("Scale"),
          viaVecMath.GetScale3D(),
          original.GetScale3D(),
          1e-18);
    });
  });
}
