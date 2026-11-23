#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "AnimInstHumanBase.generated.h"

class AHumanCharacter;
class AWeapon;
class UCombatComponent;
class UMAMovementComponent;
class UFragWeapon;
class USkeletalMeshComponent;
class UAnimMontage;

/**
 * 人类动画实例基类（1P 与 3P 共用）
 * 在标准坐标系 (X=前后, Y=左右, Z=上下) 下统一计算【机瞄过渡 + 呼吸步伐摆动 + 开火后坐力】，
 * 汇总后交由子类转换到各自基准骨骼局部空间，最终输出给 Control Rig 的 Ctrl_WeaponPivot。
 * TODO 以后可能会改为手 K 叠加动画 + 程序化弹簧，因为某些武器如沙鹰动作非常有特色，动画负责主主要动作（大幅抬枪），程序化参数都调小只增加随机化。
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

	// 开火时由 GC_Weapon_Fire 触发，向 6 自由度后坐力弹簧注入单发初速度冲量
	void AddRecoilImpulse(const UFragWeapon* FragWeapon);

protected:
	// 将标准坐标系偏移转换到基准骨骼局部空间（由 1P/3P 子类重写）
	virtual FTransform BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FRotator& StdRotation)
	{
		return FTransform(StdRotation, StdLocation, FVector::OneVector);
	}

	// 瞄准偏移缩放比例（1P 为 1.0 精确对齐摄像机准星；3P 按需缩小避免贴脸穿模）
	virtual float GetADSOffsetScale() const { return 1.f; }

	// 呼吸步伐晃动与后坐力的整体幅度缩放比例（1P 为 1.0；3P 远观适当放大以增强表现力）
	virtual float GetProceduralMotionScale() const { return 1.f; }

	// 获取当前动画实例对应的武器网格体（1P 为 EquipmentMesh1P，3P 为 EquipmentMesh），供挂点坐标映射使用
	virtual USkeletalMeshComponent* GetTargetWeaponMesh(const AWeapon* Weapon) const { return nullptr; }

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

	// 作用于 Control Rig 父控制器 Ctrl_WeaponPivot 的汇总变换偏移（机瞄 + 呼吸/移动晃动 + 开火后坐力）
	UPROPERTY(BlueprintReadOnly)
	FTransform WeaponPivotOffsetTransform = FTransform::Identity;

	// 泵动武器（如 M870）拉护木时左手在 Ctrl_WeaponPivot 局部空间下的附加平移偏移（单位: cm）
	UPROPERTY(BlueprintReadOnly)
	FVector LeftHandOffsetLocation = FVector::ZeroVector;

	// 瞄准线性过渡权重 (0 = 腰射, 1 = 完全瞄准 | 用于驱动包络线相位与后坐力收紧插值)
	UPROPERTY(BlueprintReadOnly)
	float AimAlpha = 0.f;

	// 经过非线性缓动 (EaseOut / EaseInOut) 后的瞄准平滑权重 (0 = 腰射, 1 = 完全瞄准 | 用于驱动空间位置与旋转插值)
	UPROPERTY(BlueprintReadOnly)
	float SmoothAimAlpha = 0.f;

	// 双臂 IK / Control Rig 整体权重 (持握装备时为 1，无装备时平滑过渡到 0)
	UPROPERTY(BlueprintReadOnly)
	float WeaponIKAlpha = 1.f;

	// 主线程缓存的当前武器 ADS 目标偏移（由 1P/3P 子类各自计算写入）
	FVector CachedADSOffsetLoc = FVector::ZeroVector;
	FRotator CachedADSOffsetRot = FRotator::ZeroRotator;

	// 主线程缓存的当前动作蒙太奇播放状态（供子类与动画工作线程安全读取）
	bool bIsPlayingActionMontage = false;

private:
	// 主线程缓存的角色装备状态（供动画工作线程安全读取）
	bool bHasEquipment = false;
	bool bHasWeapon = false;
	bool bRawIsAiming = false;
	float CachedADSTime = 0.2f;

	// 主线程缓存的武器程序化后坐力配置参数（供动画工作线程步进求解弹簧时安全读取）
	FVector CachedRecoilMaxLoc = FVector::ZeroVector;
	FRotator CachedRecoilMaxRot = FRotator::ZeroRotator;
	float CachedRecoilKickSpeedMul = 1.f;
	float CachedRecoilRecoverySpeedMul = 1.f;
	float CachedRecoilDampingMul = 1.f;
	float CachedADSRecoilScale = 0.5f;

	// 程序化呼吸与移动晃动状态
	float SwayPhase = 0.f;
	FVector CurrentSwayLocation = FVector::ZeroVector;
	FRotator CurrentSwayRotation = FRotator::ZeroRotator;

	// 程序化开火后坐力（6 自由度非对称二阶弹簧状态）
	FVector CurrentRecoilLocation = FVector::ZeroVector;
	FRotator CurrentRecoilRotation = FRotator::ZeroRotator;
	FVector RecoilLocVelocity = FVector::ZeroVector;
	FRotator RecoilRotVelocity = FRotator::ZeroRotator;
	int32 RecoilShotCounter = 0;

	// 主线程缓存的拉泵运动配置与护木满行程向量（Ctrl_WeaponPivot 局部空间，供动画工作线程无锁读取）
	bool bHasPumpMotion = false;
	FWeaponFirePartMotion CachedPumpMotion;
	FVector CachedPumpPivotOffset = FVector::ZeroVector;
	TWeakObjectPtr<const UAnimMontage> PrevActiveMontage;

	// 程序化拉泵运行态（单一计时器驱动左手偏移与枪身拉扯/推膛次级后坐力）
	float PumpElapsedTime = -1.f;
	float PumpImpulseScale = 1.f;
	uint8 PumpImpulseStage = 0; // 0=待后拉顿挫, 1=待前推顿挫, 2=本轮冲量已完成
};