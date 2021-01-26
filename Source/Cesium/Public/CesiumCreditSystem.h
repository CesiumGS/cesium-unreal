#pragma once 

#include <memory>
#include "GameFramework/Actor.h"
#include "CesiumCreditSystem.generated.h"

namespace Cesium3DTiles {
    class CreditSystem;
}

UCLASS()
class CESIUM_API ACesiumCreditSystem : public AActor 
{
	GENERATED_BODY()

public:
	static ACesiumCreditSystem* GetDefaultForActor(AActor* Actor);

	ACesiumCreditSystem();
	virtual ~ACesiumCreditSystem();

    /**
	 * Credits text
	 *
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Credits = "";

    // Called every frame
	virtual bool ShouldTickIfViewportsOnly() const override;
    virtual void Tick(float DeltaTime) override;

	const std::shared_ptr<Cesium3DTiles::CreditSystem>& GetExternalCreditSystem() const {
		return _pCreditSystem;
	}

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	// the underlying cesium-native credit system that is managed by this actor. 
    std::shared_ptr<Cesium3DTiles::CreditSystem> _pCreditSystem;
};