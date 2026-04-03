#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutantGhost.generated.h"

UCLASS()
class MUTATEARENA_API AMutantGhost : public AMutantCharacter
{
	GENERATED_BODY()

public:
	AMutantGhost(const FObjectInitializer& ObjectInitializer);

	void ShowOverheadWidget(bool bIsShow);

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

	virtual void OnASCInit() override;
	void OnJumpZVelocityChanged(const FOnAttributeChangeData& Data);
	
};
