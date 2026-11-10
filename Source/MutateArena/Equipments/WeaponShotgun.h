#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "WeaponShotgun.generated.h"

UCLASS()
class MUTATEARENA_API AWeaponShotgun : public AWeapon
{
	GENERATED_BODY()

public:
	AWeaponShotgun();

	virtual FVector CalcFinalTarget(float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw) const override;
	virtual void SpawnProjectile(const FVector& FinalTarget, int32 ShotCount, int32 SpreadSeed) override;

};
