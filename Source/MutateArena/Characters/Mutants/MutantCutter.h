#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutantCutter.generated.h"

UCLASS()
class MUTATEARENA_API AMutantCutter : public AMutantCharacter
{
	GENERATED_BODY()

public:
	AMutantCutter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void LightAttackButtonPressed(const FInputActionValue& Value) override;
	virtual void LightAttackButtonReleased(const FInputActionValue& Value) override;
	virtual void HeavyAttackButtonPressed(const FInputActionValue& Value) override;
	virtual void HeavyAttackButtonReleased(const FInputActionValue& Value) override;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> BleedEffectClass;

public:
	virtual void ServerApplyDamage_Implementation(AActor* OtherActor, float Damage) override;
	
};
