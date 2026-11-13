#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "AnimInstHumanBase.generated.h"

class AHumanCharacter;
class UCombatComponent;
class UMAMovementComponent;
class UFragWeapon;

/**
 * 人类动画实例基类（1P 与 3P 共用）
 * 在标准坐标系 (X=前后, Y=左右, Z=上下) 下计算【机瞄 + 呼吸步伐摆动 + 开火后坐力】，
 * 汇总后交由子类转换到骨骼局部空间，输出给 Control Rig 的 Ctrl_WeaponPivot。
 */
UCLASS(Abstract)
class MUTATEARENA_API UAnimInstHumanBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstHumanBase();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	// 开火时由 GC_Weapon_Fire 触发，向 Control Rig 施加单发后坐力冲量
	void AddRecoilImpulse(const UFragWeapon* FragWeapon);

protected:
	// 将标准坐标系偏移转换到基准骨骼局部空间（由 1P/3P 子类重写）
	virtual FTransform BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FRotator& StdRotation)
	{
		return FTransform(StdRotation, StdLocation, FVector::OneVector);
	}

	// 瞄准偏移缩放比例（1P 为 1.0 精确对齐准星；3P 按需缩小避免穿模）
	virtual float GetADSOffsetScale() const { return 1.f; }

	// 呼吸晃动与后坐力的整体幅度缩放比例（1P 为 1.0；3P 远观幅度略大）
	virtual float GetProceduralMotionScale() const { return 1.f; }

	UPROPERTY()
	TObjectPtr<AHumanCharacter> HumanChar;
	UPROPERTY()
	TObjectPtr<UCombatComponent> CombatComp;
	UPROPERTY()
	TObjectPtr<UMAMovementComponent> MovementComp;

	UPROPERTY(BlueprintReadOnly)
	bool bIsOnLadder = false;
	UPROPERTY(BlueprintReadOnly)
	float Speed = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float WalkPlayRate = 1.f;
	UPROPERTY(BlueprintReadOnly)
	bool bIsInAir = false;
	UPROPERTY(BlueprintReadOnly)
	bool bIsAccelerating = false;
	UPROPERTY(BlueprintReadOnly)
	bool bIsCrouched = false;
	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming = false;
	UPROPERTY(BlueprintReadOnly)
	EEquipmentName EquipmentName = EEquipmentName::None;

	// 作用于 Control Rig 父控制器 Ctrl_WeaponPivot 的汇总变换偏移（瞄准 + 呼吸/移动晃动 + 开火后坐力）
	UPROPERTY(BlueprintReadOnly)
	FTransform WeaponPivotOffsetTransform = FTransform::Identity;

	// 瞄准平滑过渡权重 (0 = 腰射, 1 = 完全瞄准)
	UPROPERTY(BlueprintReadOnly)
	float AimAlpha = 0.f;

	// 武器双臂 IK / Control Rig 整体权重 (持握武器时为 1，空手时平滑过渡到 0)
	UPROPERTY(BlueprintReadOnly)
	float WeaponIKAlpha = 1.f;

private:
	// 主线程缓存的当前武器状态（供动画工作线程安全读取）
	bool bHasEquipment = false;
	bool bHasWeapon = false;
	bool bRawIsAiming = false;
	bool bIsPlayingActionMontage = false;
	float CachedADSTime = 0.2f;
	FVector CachedADSOffsetLoc = FVector::ZeroVector;
	FRotator CachedADSOffsetRot = FRotator::ZeroRotator;

	// 主线程缓存的武器程序化后坐力配置参数（实际值在持枪时直接从 UFragWeapon 同步）
	FVector CachedRecoilMaxLoc = FVector::ZeroVector;
	FRotator CachedRecoilMaxRot = FRotator::ZeroRotator;
	float CachedRecoilKickSpeedMul = 1.f;
	float CachedRecoilRecoverySpeedMul = 1.f;
	float CachedRecoilDampingMul = 1.f;
	float CachedRecoilNoiseScale = 1.f;

	// 程序化瞄准状态
	FVector CurrentADSLocation = FVector::ZeroVector;
	FRotator CurrentADSRotation = FRotator::ZeroRotator;

	// 程序化呼吸与移动晃动状态
	float SwayPhase = 0.f;
	FVector CurrentSwayLocation = FVector::ZeroVector;
	FRotator CurrentSwayRotation = FRotator::ZeroRotator;

	// 程序化开火后坐力（6自由度独立二阶欠阻尼弹簧状态）
	FVector CurrentRecoilLocation = FVector::ZeroVector;
	FRotator CurrentRecoilRotation = FRotator::ZeroRotator;
	FVector RecoilLocVelocity = FVector::ZeroVector;
	FRotator RecoilRotVelocity = FRotator::ZeroRotator;
	float RecoilNoiseTime = 0.f;
	int32 RecoilShotCounter = 0;
};