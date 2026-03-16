#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "InteractorComponent.generated.h"

UCLASS()
class MUTATEARENA_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractorComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void TraceInteractTarget(FHitResult& OutHit);

	void InteractStarted(const FInputActionValue& Value);
	void InteractOngoing(const FInputActionValue& Value);
	void InteractTriggered(const FInputActionValue& Value);
	void InteractCompleted(const FInputActionValue& Value);
	void InteractCanceled(const FInputActionValue& Value);

protected:
	UFUNCTION(Server, Reliable)
	void ServerInteractTriggered(AActor* TempInteractTarget);

	UPROPERTY()
	AActor* InteractTarget;
	
};
