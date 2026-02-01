#pragma once

#include "CoreMinimal.h"
#include "Equipment.h"
#include "Weapon.generated.h"

UCLASS()
class MUTATEARENA_API AWeapon : public AEquipment
{
	GENERATED_BODY()

public:
	AWeapon();
	
	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* ScopeCapture;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AShell> ShellClass;
	
	UPROPERTY(EditAnywhere)
	UAnimMontage* FireMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* FireMontage_E;
	UPROPERTY(EditAnywhere)
	UAnimMontage* FireADSMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* FireADSMontage_E;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ReloadMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* ReloadMontage_E;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ADSMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* ADSMontage_E;

	UPROPERTY()
	float AimingFOVMul = 0.9; // 缩放倍数
	UPROPERTY()
	bool bIsPIP = false;
	UPROPERTY()
	float ScopeFOV = 90.f;
	UPROPERTY()
	int32 CarriedAmmo; // 当前携弹量
	UPROPERTY()
	int32 MaxCarriedAmmo; // 最大携弹量
	UPROPERTY()
	int32 Ammo; // 当前弹匣子弹数量
	UPROPERTY()
	int32 MagCapacity; // 弹匣容量
	UPROPERTY()
	float FireRate;
	UPROPERTY()
	bool bIsAutomatic;
	UPROPERTY()
	int32 PelletNum;
	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return Ammo == MagCapacity; }
	FORCEINLINE float GetFireDelay() const { return 60 / FireRate; }
	
	UPROPERTY()
	class UCurveVector* RecoilCurve;
	UPROPERTY()
	UCurveFloat* RecoilCurveRandVert;
	UPROPERTY()
	UCurveFloat* RecoilCurveRandHor;
	// 应用后坐力需要的时间
	UPROPERTY()
	float RecoilIncTime;
	// 恢复后坐力需要的时间
	UPROPERTY()
	float RecoilMaxDecTime;
	UPROPERTY()
	float RecoilMinDecTime;
	// 恢复后坐力需要的时间 达到最大时所需要的垂直后坐力
	UPROPERTY()
	float RecoilVertRef_DecTime;
	// 准星达到最大时所需要的垂直后坐力
	UPROPERTY()
	float RecoilVertRef_Crosshair;
	// 准星
	UPROPERTY()
	float CrosshairBaseSpread;
	
	void SetScopeActive(bool bIsActive);

protected:
	virtual void BeginPlay() override;
	
	void InitData();

	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* MechSound;
public:
	virtual void Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw);
	virtual void SetAmmo(int32 AmmoNum);
	virtual void SetCarriedAmmo(int32 AmmoNum);
protected:
	void SpendRound();
	void SetHUDAmmo();
	void SetHUDCarriedAmmo();
	
public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetFullAmmo();
	
};
