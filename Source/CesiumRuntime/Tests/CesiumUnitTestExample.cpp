// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CoreTypes.h"
#include "Misc/AutomationTest.h"

// TODO Is it OK to hop through paths to test private classes?
#include "../Private/CesiumTransforms.h"

#if WITH_DEV_AUTOMATION_TESTS

/*
 * A trivial example for unit/API tests that do not require any
 * of the actual Unreal Editor infrastructure. These are
 * essentially tests for objects that can be instantiated
 * and where methods can be called, to check the pre- and
 * postconditions.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumUnitTestExample,
    "Cesium.Examples.UnitTestExample",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::SmokeFilter)

bool FCesiumUnitTestExample::RunTest(const FString& Parameters) {
  // A trivial unit test for equality (that passes)
  {
    const double centimeters = 123.456;
    const double actualMeters =
        CesiumTransforms::centimetersToMeters * centimeters;
    const double expectedMeters = 1.23456;

    TestEqual(
        TEXT("Converting centimeters to meters scaled with 0.01"),
        actualMeters,
        expectedMeters);
  }

  // A trivial unit test for equality (that INTENTIONALLY fails)
  /*
  {
          const double centimeters = 123.456;
          // INTENTIONALLY using the wrong factor here:
          const double actualMeters = CesiumTransforms::metersToCentimeters *
  centimeters; const double expectedMeters = 1.23456;

          TestEqual(TEXT("Converting centimeters to meters, INTENTIONALLY
  failing"), actualMeters, expectedMeters);
  }
  */
  return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
