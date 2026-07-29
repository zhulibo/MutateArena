#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Equipments/Equipment.h"
#include "Throwing.generated.h"

UCLASS()
class MUTATEARENA_API AThrowing : public AEquipment
{
	GENERATED_BODY()

public:
	AThrowing();
	
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ThrowMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* ThrowMontage_E;
	virtual void ThrowOut();
	
	virtual void OnRep_EquipmentState(EEquipmentState OldState) override;
	
protected:
	bool bHasBounced = false;
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* HitSound;
	UFUNCTION()
	virtual void OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
	
};
