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

	virtual void Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw) override;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectileBullet> ProjectileClass;

};
