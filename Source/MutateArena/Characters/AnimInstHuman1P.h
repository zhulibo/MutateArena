#pragma once

#include "CoreMinimal.h"
#include "AnimInstHumanBase.h"
#include "AnimInstHuman1P.generated.h"

/**
 * 第一人称人类手臂动画实例 (Mesh1P / Blender FPArms)
 * 继承自 UAnimInstHumanBase：
 * - 第一人称枪械统一挂载在 weapon_root 骨骼的 WeaponRoot 插槽上。
 * - 在 Blender 制作各枪械 Idle 动画时，直接移动并旋转了 weapon_root 骨骼将枪放到手上。
 * - 重写 BuildWeaponPivotOffsetTransform，结合 weapon_root 当前动画帧的骨骼朝向，将 UFragWeapon 中按
 *   UE 标准语义坐标系 (X=前后, Y=左右, Z=上下) 配置的参数动态转换到 weapon_root 局部空间。
 */
UCLASS(Abstract)
class MUTATEARENA_API UAnimInstHuman1P : public UAnimInstHumanBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	virtual FTransform BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FRotator& StdRotation) override;

private:
	// 主线程缓存的 weapon_root 纯动画组件空间旋转（已剥离上一帧 Control Rig 写回的旋转）
	FQuat CachedWeaponRootCompRot = FQuat::Identity;
	// 上一帧 Control Rig 写回给 weapon_root 的组件空间旋转，用于下一帧还原纯动画朝向
	FQuat LastAppliedCompRot = FQuat::Identity;
};
