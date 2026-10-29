#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Equipment.h"
#include "Throwing.generated.h"

class UFragThrowing;

UCLASS()
class MUTATEARENA_API AThrowing : public AEquipment
{
	GENERATED_BODY()

public:
	AThrowing();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(Transient)
	TObjectPtr<const UFragThrowing> FragThrowing = nullptr;

	virtual void ThrowOut();

	virtual void OnRep_EquipmentState(EEquipmentState OldState) override;

protected:
	bool bHasBounced = false;
	UFUNCTION()
	virtual void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	
};
