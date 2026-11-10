#pragma once

#include "CoreMinimal.h"
#include "Equipment.h"
#include "Data/EquipmentAsset.h"
#include "Weapon.generated.h"

class UFragWeapon;
class USceneCaptureComponent2D;

UCLASS()
class MUTATEARENA_API AWeapon : public AEquipment
{
	GENERATED_BODY()

public:
	AWeapon();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	
	UPROPERTY(Transient)
	TObjectPtr<const UFragWeapon> FragWeapon = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo; // 当前携弹量
	UPROPERTY(ReplicatedUsing = OnRep_Ammo)
	int32 Ammo; // 当前弹匣子弹数量

	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return Ammo == FragWeapon->MagCapacity; }
	FORCEINLINE float GetFireDelay() const { return 60 / FragWeapon->FireRate; }

protected:
	UFUNCTION()
	void OnRep_Ammo();
	UFUNCTION()
	void OnRep_CarriedAmmo();

public:
	float LastFireTime = 0.f;
	virtual FVector CalcFinalTarget(float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw) const;
	bool GetFireLocations(FVector& OutSpawnLocation, FVector& OutMuzzleLocation) const;
	float CalcVisualBlendDistance(const FVector& SpawnLocation, const FVector& ShotDirection, float MaxBlendDistance = 1000.f) const;
	virtual void Fire(const FVector& FinalTarget, int32 SpreadSeed);
	virtual void SpawnProjectile(const FVector& FinalTarget, int32 ShotCount, int32 SpreadSeed) {}
	void EjectShell();
	virtual void SetAmmo(int32 AmmoNum);
	virtual void SetCarriedAmmo(int32 AmmoNum);
protected:
	void SpendRound();
	void SetHUDAmmo();
	void SetHUDCarriedAmmo();
	
};
