#pragma once

#include "CoreMinimal.h"
#include "AnimInstHumanBase.h"
#include "AnimInstHuman.generated.h"

/**
 * 第三人称人类全身动画实例 (BodyMesh / MetaHuman)
 * 继承自 UAnimInstHumanBase：
 * - 第三人称各枪械在 hand_r 上建有独立插槽，关卡序列制作动画时旋转了 hand_r 与插槽来适配双手握枪。
 * - 重写 BuildWeaponPivotOffsetTransform，结合虚拟骨骼 VB spine_05_hand_r (即 hand_r) 当前帧的动画朝向与角色俯仰角 AimPitch，
 *   将 UFragWeapon 中按 UE 标准语义坐标系 (X=前后, Y=左右, Z=上下) 配置的参数动态转换到 Ctrl_WeaponPivot 局部空间。
 */
UCLASS(Abstract)
class MUTATEARENA_API UAnimInstHuman : public UAnimInstHumanBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	virtual FTransform BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FQuat& StdRotation) const override;
	virtual USkeletalMeshComponent* GetTargetWeaponMesh(const AWeapon* Weapon) const override;

	// 第三人称已有瞄准偏移 AO，机瞄位移与动态弧线按 50% 缩放即可自然表现举枪贴腮，避免动作幅度过大或枪托穿插胸口
	virtual float GetADSOffsetScale() const override { return 0.5f; }
	// 第三人称从远距离观察，呼吸晃动与开火后坐力幅度适当放大以便其他玩家清晰感知
	virtual float GetProceduralMotionScale() const override { return 1.3f; }

protected:
	UPROPERTY(BlueprintReadOnly)
	float AimYaw = 0.f;
	UPROPERTY(BlueprintReadOnly)
	float AimPitch = 0.f;

	// 3P 瞄准时分配给 neck_01 (40%) 与 neck_02 (60%) 的组件空间附加旋转
	UPROPERTY(BlueprintReadOnly, Category = "ADS")
	FRotator Neck01ADSRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "ADS")
	FRotator Neck02ADSRotation = FRotator::ZeroRotator;

private:
	// 主线程缓存的 VB spine_05_hand_r 动画组件空间旋转
	FQuat CachedHandBoneCompRot = FQuat::Identity;
	// 主线程缓存的当前武器 3P 歪头贴腮旋转
	FRotator CachedADSHeadTiltRot = FRotator::ZeroRotator;
};
