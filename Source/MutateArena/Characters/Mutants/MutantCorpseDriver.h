#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutantCorpseDriver.generated.h"

UCLASS()
class MUTATEARENA_API AMutantCorpseDriver : public AMutantCharacter
{
	GENERATED_BODY()

public:
	AMutantCorpseDriver(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void LightAttackButtonPressed(const FInputActionValue& Value) override;
	virtual void LightAttackButtonReleased(const FInputActionValue& Value) override;
	virtual void HeavyAttackButtonPressed(const FInputActionValue& Value) override;
	virtual void HeavyAttackButtonReleased(const FInputActionValue& Value) override;

protected:
	virtual void OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	virtual void OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

};
