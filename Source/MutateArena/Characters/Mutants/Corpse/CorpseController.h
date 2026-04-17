#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "CorpseController.generated.h"

class UBehaviorTree;
class UAISenseConfig_Sight;

UCLASS()
class MUTATEARENA_API ACorpseController : public AAIController
{
	GENERATED_BODY()

public:
	ACorpseController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditAnywhere)
	UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere)
	class UAIPerceptionComponent* AIPerceptionComp;
	
	UPROPERTY(VisibleAnywhere)
	UAISenseConfig_Sight* SightConfig;

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);
	
};
