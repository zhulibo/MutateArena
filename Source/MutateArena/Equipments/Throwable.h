#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Equipment.h"
#include "Throwable.generated.h"

class UFragThrowable;

UCLASS()
class MUTATEARENA_API AThrowable : public AEquipment
{
	GENERATED_BODY()

public:
	AThrowable();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(Transient)
	TObjectPtr<const UFragThrowable> FragThrowable = nullptr;

	virtual void ThrowOut();

	virtual void OnRep_EquipmentState(EEquipmentState OldState) override;

protected:
	bool bHasBounced = false;
	UFUNCTION()
	virtual void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	
};
