#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "AnimInstEquipment.generated.h"

class AEquipment;
class AWeapon;

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstEquipment : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstEquipment();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	// 开火时由 GC_Weapon_Fire 触发，重置已配置机械构件的往复运动计时器
	UFUNCTION(BlueprintCallable)
	void TriggerFireMotion();

protected:
	static constexpr int32 MechBoneCount = static_cast<int32>(EWeaponMechBone::None);

	UPROPERTY()
	TObjectPtr<AEquipment> Equipment;

	UPROPERTY()
	TObjectPtr<AWeapon> OwningWeapon;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming = false;

	// 7 组标准机械骨骼的程序化开火往复驱动参数（Alpha 为 0 时由 AnimGraph 节点直接短路跳过）
	UPROPERTY(BlueprintReadOnly)
	FVector BarrelLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FRotator BarrelRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly)
	float BarrelAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector TriggerLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FRotator TriggerRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly)
	float TriggerAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector ChargingHandleLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FRotator ChargingHandleRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly)
	float ChargingHandleAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector BoltLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FRotator BoltRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly)
	float BoltAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector PumpLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FRotator PumpRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly)
	float PumpAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector DustCoverLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FRotator DustCoverRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly)
	float DustCoverAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FVector SlideLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FRotator SlideRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly)
	float SlideAlpha = 0.f;

private:
	// 主线程缓存的各机械骨骼运动配置、启用标志与蒙太奇状态，供动画工作线程安全读取
	FWeaponFirePartMotion CachedMotions[MechBoneCount];
	bool bBoneConfigured[MechBoneCount] = {};
	bool bIsPlayingWeaponMontage = false;

	// 播放换弹等武器蒙太奇时的平滑抑制权重（1=正常驱动，0=交还控制权给蒙太奇）
	float MontageSuppressAlpha = 1.f;

	// 各骨骼通道往复运动计时器（>= 0 表示运动中，-1 表示静止）
	float ElapsedTime[MechBoneCount] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };

	// 各骨骼通道当前帧的实际无衰减相位 (0.0 ~ 1.0)，用于连发中途再次触发时无缝接续起点
	float CurrentPhase[MechBoneCount] = {};

	// 各骨骼通道本次开火去程的起始相位（若在半途如 0.33 处再次开火，则直接从 0.33 插值向 1.0）
	float KickStartPhase[MechBoneCount] = {};
};
