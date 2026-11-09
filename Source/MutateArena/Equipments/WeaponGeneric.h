#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "WeaponGeneric.generated.h"

UCLASS()
class MUTATEARENA_API AWeaponGeneric : public AWeapon
{
	GENERATED_BODY()

public:
	AWeaponGeneric();

	virtual void SpawnProjectile(const FVector& FinalTarget, int32 ShotCount) override;

};
