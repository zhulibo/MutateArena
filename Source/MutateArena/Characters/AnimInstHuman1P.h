#pragma once

#include "CoreMinimal.h"
#include "AnimInstHumanBase.h"
#include "AnimInstHuman1P.generated.h"

/**
 * 第一人称人类手臂动画实例 (Mesh1P / Blender FPArms)
 * 继承自 UAnimInstHumanBase：
 * - 第一人称枪械统一挂载在 weapon_root 骨骼的 WeaponRoot 插槽上。
 * - 在 Blender 制作各枪械 Idle 动画时，直接移动并旋转了 weapon_root 骨骼将枪放到手上。
 * - 通过读取虚拟骨骼 VB root_weapon_root（Control Rig 不修改该骨骼）获取当前帧纯动画基准姿态，
 *   重写 BuildWeaponPivotOffsetTransform 将标准语义坐标系 (X=前后, Y=左右, Z=上下) 动态转换到 weapon_root 局部空间。
 */
UCLASS(Abstract)
class MUTATEARENA_API UAnimInstHuman1P : public UAnimInstHumanBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	virtual FTransform BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FQuat& StdRotation) const override;
	virtual USkeletalMeshComponent* GetTargetWeaponMesh(const AWeapon* Weapon) const override;

private:
	// 主线程缓存的 VB root_weapon_root 纯动画组件空间旋转
	FQuat CachedWeaponRootCompRot = FQuat::Identity;
};
