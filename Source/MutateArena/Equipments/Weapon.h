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

	UPROPERTY(EditAnywhere)
	UAnimMontage* FireMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* FireMontage_E;
	UPROPERTY(EditAnywhere)
	UAnimMontage* FireADSMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* FireADSMontage_E;
	virtual void Fire(const FVector& HitTarget, float RecoilVert, float RecoilHor);

	UPROPERTY(EditAnywhere)
	UAnimMontage* ReloadMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* ReloadMontage_E;
	virtual void SetAmmo(int32 AmmoNum);
	virtual void SetCarriedAmmo(int32 AmmoNum);

	UPROPERTY(EditAnywhere)
	UAnimMontage* ADSMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* ADSMontage_E;
	UPROPERTY(EditAnywhere)
	UAnimMontage* ADSReverseMontage_C;

	// 后坐力范围
	UPROPERTY()
	float RecoilMaxVert;
	UPROPERTY()
	float RecoilMinVert;
	UPROPERTY()
	float RecoilMaxHor;
	UPROPERTY()
	float RecoilMinHor;

	// 首发后坐力倍率
	UPROPERTY()
	float FirstShotRecoilMul;

	// 应用后坐力需要的时间
	UPROPERTY()
	float RecoilIncTime;

	// 总后坐力上限
	UPROPERTY()
	float RecoilTotalVertLimit;
	UPROPERTY()
	float RecoilTotalHorLimit;

	// 后坐力回复速度
	UPROPERTY()
	float RecoilDecSpeed;

	// 子弹散布，固定值，不受其他因素影响，固定存在
	UPROPERTY()
	float CenterSpread;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFullAmmo();

	void SetScopeActive(bool bIsActive);

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	const USkeletalMeshSocket* MuzzleSocket;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AShell> ShellClass;
	
	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* ScopeCapture;

	UPROPERTY(EditAnywhere)
	bool bHasScope = false;

	UPROPERTY()
	float AimingFOVMul = 0.9; // 缩放倍数
	UPROPERTY()
	float AimSpeed = 30.f;
	UPROPERTY()
	float AimMoveSpeedMul = 1.f;
	UPROPERTY()
	float ScopeFOV = 1.f;

	UPROPERTY()
	int32 MaxCarriedAmmo; // 最大携弹量
	UPROPERTY()
	int32 CarriedAmmo; // 当前携弹量

	UPROPERTY()
	int32 MagCapacity; // 弹匣容量
	UPROPERTY()
	int32 Ammo; // 当前弹匣子弹数量
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* MechSound;

	UPROPERTY()
	float FireRate;
	UPROPERTY()
	bool bIsAutomatic;

	UPROPERTY()
	int32 PelletNum;
	
	void InitData();

	void SpendRound();
	void SetHUDAmmo();
	void SetHUDCarriedAmmo();

public:
	FORCEINLINE float GetAimingFOVMul() const { return AimingFOVMul; }
	FORCEINLINE float GetAimSpeed() const { return AimSpeed; }
	FORCEINLINE float GetAimMoveSpeedMul() const { return AimMoveSpeedMul; }
	FORCEINLINE float GetbHasScope() const { return bHasScope; }

	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
	FORCEINLINE int32 GetMaxCarriedAmmo() const { return MaxCarriedAmmo; }

	FORCEINLINE bool IsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool IsFull() const { return Ammo == MagCapacity; }

	FORCEINLINE float GetFireDelay() const { return 60 / FireRate; }
	FORCEINLINE bool IsAutomatic() const { return bIsAutomatic; }

	FORCEINLINE int32 GetPelletNum() const { return PelletNum; }

};
