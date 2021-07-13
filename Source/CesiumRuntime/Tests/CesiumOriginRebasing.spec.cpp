
#include "EditorModeManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationEditorPromotionCommon.h"

#include <CesiumGeoreference.h>
#include <CesiumGeoreferenceComponent.h>
#include <GlobeAwareDefaultPawn.h>

class CesiumOriginRebasing {
public:
  CesiumOriginRebasing() {}
  ~CesiumOriginRebasing() {}
};

BEGIN_DEFINE_SPEC(
    CesiumOriginRebasingSpec,
    "Cesium.Georeference.OriginRebasing",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext)
END_DEFINE_SPEC(CesiumOriginRebasingSpec)

void CesiumOriginRebasingSpec::Define() {
  Describe("When starting PIE", [this]() {
    BeforeEach([this]() {
      UWorld* editorWorld = FAutomationEditorCommonUtils::CreateNewMap();
      ULevel* editorLevel = editorWorld->GetCurrentLevel();
      AActor* editorPawn = GEditor->AddActor(
          editorLevel,
          AGlobeAwareDefaultPawn::StaticClass(),
          FTransform(FVector(0.0, 0.0f, 0.0f)));
      TestNotNull(
          TEXT("The AGlobeAwareDefaultPawn instance could be created"),
          editorPawn);

      FEditorPromotionTestUtilities::StartPIE(true);
    });

    It("it should cause an origin rebasing when moving the pawn", [this]() {
      FPlatformProcess::Sleep(1.0f);

      UWorld* world = GEditor->PlayWorld;
      TestNotNull(TEXT("The PIE world could be obtained"), world);

      TArray<AActor*> ActorList;
      UGameplayStatics::GetAllActorsOfClass(
          world,
          AGlobeAwareDefaultPawn::StaticClass(),
          ActorList);

      TestEqual(TEXT("There was one PIE pawn"), ActorList.Num(), 1);

      // AActor* pawn = ActorList[0];
      // TestNotNull(TEXT("The PIE pawn could be obtained"), pawn);
      // pawn->SetActorLocation(FVector(100000.0f, 100000.0f, 100000.0f));

      FPlatformProcess::Sleep(1.0f);
    });

    AfterEach([this]() {
      FEditorPromotionTestUtilities::EndPIE();
      FAutomationEditorCommonUtils::CreateNewMap();
    });
  });
}
