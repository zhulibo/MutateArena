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
	
	virtual void BeginPlay() override;
	
	void StartAFKCheck();
	void UpdateActiveTime();
	UFUNCTION()
	AActor* GetBestPerceivedTarget();

protected:
	UPROPERTY()
	class ABaseCharacter* BaseChar;
	UPROPERTY()
	class UStateTreeComponent* StateTreeComp;
	UPROPERTY()
	class UAIPerceptionComponent* AIPerceptionComp;
	
	FTimerHandle AFKCheckTimerHandle;
	bool bIsAutoHosting = false;
	
	UFUNCTION()
	void CheckIdleStatus();

	void StartAutoHost();
	void StopAutoHost();

	void BroadcastAFKUIState(bool bIsHosting);
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
};
