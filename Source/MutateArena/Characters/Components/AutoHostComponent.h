#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AutoHostComponent.generated.h"

struct FAIStimulus;

UCLASS()
class MUTATEARENA_API UAutoHostComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAutoHostComponent();
	
	void StartAFKCheck();
	void UpdateActiveTime();
	UFUNCTION()
	AActor* GetBestPerceivedTarget();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	class ABaseCharacter* BaseChar;
	UPROPERTY()
	class UStateTreeComponent* StateTreeComp;
	UPROPERTY()
	class UAIPerceptionComponent* AIPerceptionComp;

	float LastActiveTime;
	FTimerHandle AFKCheckTimerHandle;
	bool bIsAutoHosting;
	
	UFUNCTION()
	void CheckIdleStatus();

	void StartAutoHost();
	void StopAutoHost();

	void BroadcastAFKUIState(bool bIsHosting);
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
};
