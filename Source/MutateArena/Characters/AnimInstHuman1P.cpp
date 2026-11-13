#include "AnimInstHuman1P.h"

#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/MutateArena.h"

void UAnimInstHuman1P::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 读取 weapon_root 在组件空间中的旋转，并剥离上一帧 Control Rig (Set Transform - Bone) 写回的附加旋转，
	// 还原 Blender 动画本身赋予 weapon_root 的真实朝向
	if (USkeletalMeshComponent* SkelMesh = GetSkelMeshComponent())
	{
		if (SkelMesh->GetBoneIndex(Bones1P::WeaponRoot) != INDEX_NONE)
		{
			const FQuat RawBoneCompRot = SkelMesh->GetBoneQuaternion(Bones1P::WeaponRoot, EBoneSpaces::ComponentSpace);
			CachedWeaponRootCompRot = (LastAppliedCompRot.Inverse() * RawBoneCompRot).GetNormalized();
		}
	}
}

FTransform UAnimInstHuman1P::BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FRotator& StdRotation)
{
	// 1. FPArms 网格体在角色蓝图中绕 Z 轴旋转了 -90 度，因此组件空间中：+Y = 前后(+前), -X = 左右(+右), +Z = 上下(+上)
	const FQuat StdToComp = FRotator(0.f, 90.f, 0.f).Quaternion();
	const FQuat StdQuat = StdRotation.Quaternion();
	const FQuat CompSpaceRot = StdToComp * StdQuat * StdToComp.Inverse();

	// 记录本帧实际写入 weapon_root 的组件空间旋转，供下一帧 NativeUpdateAnimation 剥离
	LastAppliedCompRot = FQuat::Slerp(FQuat::Identity, CompSpaceRot, WeaponIKAlpha);

	// 2. Blender 动画中移动/旋转了 weapon_root，而 CR_Human1P 中 Multiply(A, B) 会将 A 放在 B (weapon_root) 的局部空间下计算：
	//    Result.Loc = B_Rot * A_Loc + B_Loc
	//    Result.Rot = B_Rot * A_Rot
	//    因此乘以 CachedWeaponRootCompRot.Inverse() 抵消 B_Rot，使无论 weapon_root 在 Blender 里如何旋转，
	//    UFragWeapon 的 X=前后, Y=左右, Z=上下 始终严格沿着第一人称镜头的前后、左右、上下方向生效！
	const FQuat StdToBone = CachedWeaponRootCompRot.Inverse() * StdToComp;
	const FVector BoneSpaceLoc = StdToBone.RotateVector(StdLocation);
	const FQuat BoneSpaceRot = StdToBone * StdQuat * StdToBone.Inverse();

	return FTransform(BoneSpaceRot, BoneSpaceLoc, FVector::OneVector);
}
