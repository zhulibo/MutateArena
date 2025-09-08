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
	float AimingWalkSpeedMul = 1.f;
	UPROPERTY()
	bool bIsPIP = false;
	UPROPERTY()
	float ScopeFOV = 90.f;
	UPROPERTY()
	int32 MaxCarriedAmmo; // 最大携弹量
	UPROPERTY()
	int32 CarriedAmmo; // 当前携弹量
	UPROPERTY()
	int32 MagCapacity; // 弹匣容量
	UPROPERTY()
	int32 Ammo; // 当前弹匣子弹数量
	UPROPERTY()
	float FireRate;
	UPROPERTY()
	bool bIsAutomatic;
	UPROPERTY()
	int32 PelletNum;
	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return Ammo == MagCapacity; }
	FORCEINLINE float GetFireDelay() const { return 60 / FireRate; }

	// 后坐力范围
	UPROPERTY()
	float RecoilMaxVert;
	UPROPERTY()
	float RecoilMinVert;
	UPROPERTY()
	float RecoilMaxHor;
	UPROPERTY()
	float RecoilMinHor;
	// 总后坐力上限
	UPROPERTY()
	float RecoilTotalVertLimit;
	UPROPERTY()
	float RecoilTotalHorLimit;
	// 首发后坐力倍率
	UPROPERTY()
	float FirstShotRecoilMul;
	// 应用后坐力需要的时间
	UPROPERTY()
	float RecoilIncTime;
	// 后坐力回复速度
	UPROPERTY()
	float RecoilDecSpeed;
	// 子弹散布角度
	UPROPERTY()
	float CenterSpreadAngle;

	void SetScopeActive(bool bIsActive);

protected:
	virtual void BeginPlay() override;
	
	void InitData();

	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* MechSound;
public:
	virtual void Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor);
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
