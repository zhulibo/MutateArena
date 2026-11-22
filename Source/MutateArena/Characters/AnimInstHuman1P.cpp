#include "AnimInstHuman1P.h"

#include "HumanCharacter.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

void UAnimInstHuman1P::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	USkeletalMeshComponent* SkelMesh = GetSkelMeshComponent();
	const int32 WeaponRootBoneIdx = SkelMesh ? SkelMesh->GetBoneIndex(Bones::WeaponRoot) : INDEX_NONE;
	if (WeaponRootBoneIdx == INDEX_NONE)
	{
		return;
	}

	// 1. 读取 weapon_root 在组件空间中的当前变换，并剥离上一帧 Control Rig 写回的附加旋转与位移，
	//    还原 Blender 纯动画本身赋予 weapon_root 的纯净基准姿态
	const FTransform RawBoneCompTransform = SkelMesh->GetBoneTransform(WeaponRootBoneIdx, FTransform::Identity);
	CachedWeaponRootCompRot = (LastAppliedCompRot.Inverse() * RawBoneCompTransform.GetRotation()).GetNormalized();
	const FVector PureWeaponRootCompLoc = RawBoneCompTransform.GetLocation() - LastAppliedCompLoc;

	// 2. 仅在非动作蒙太奇期间（纯 Idle/移动基准姿态）计算 1P 瞄准插槽 (Sight) 到摄像机中心轴的精确偏移，
	//    避免换弹/切枪蒙太奇大幅旋转移动 weapon_root 时污染机瞄目标偏移并与动作蒙太奇互相拉扯
	if (bIsPlayingActionMontage || !HumanChar || !CombatComp) return;
	AWeapon* CurWeapon = CombatComp->GetCurWeapon();
	if (!CurWeapon || !CurWeapon->FragWeapon || !CurWeapon->EquipmentMesh1P) return;

	if (CurWeapon->EquipmentMesh1P->DoesSocketExist(Sockets::Sight))
	{
		const UFragWeapon* FragWeapon = CurWeapon->FragWeapon;

		// Mesh1P 与 Camera 同挂于胶囊体且共享相同眼高原点 (0, 0, FinalEyeZ)，Mesh1P 相对 Camera 恒定绕 Z 轴旋转 -90°。
		// 直接使用组件空间静态映射 CompToCam (Yaw -90°，与 BuildWeaponPivotOffsetTransform 中的 StdToComp 严格互逆) 转换，
		// 避免通过 GetComponentTransform() 绕行世界空间时因 Camera 在帧末 GetCameraView 才同步旋转而产生 1 帧甩鼠滞后。
		const FTransform CompToCam(FRotator(0.f, -90.f, 0.f));
		const FTransform PureBoneComp(CachedWeaponRootCompRot, PureWeaponRootCompLoc, FVector::OneVector);
		const FTransform PureBoneInCam = PureBoneComp * CompToCam;

		// 获取 Sight 插槽相对于旋转支点骨骼 weapon_root 的局部变换（若 WeaponRoot 插槽相对骨骼存在局部偏移则一并计入旋转力臂）
		FTransform SightLocalToBone = CurWeapon->EquipmentMesh1P->GetSocketTransform(Sockets::Sight, RTS_Component);
		if (const USkeletalMeshSocket* WeaponRootSocket = SkelMesh->GetSocketByName(Sockets::WeaponRoot);
			WeaponRootSocket && WeaponRootSocket->BoneName == Bones::WeaponRoot)
		{
			SightLocalToBone = SightLocalToBone * WeaponRootSocket->GetSocketLocalTransform();
		}

		// 武器网格体局部空间为 -Y=前, +X=右, +Z=上，而标准/Camera坐标系为 +X=前, +Y=右, +Z=上：
		// 右乘 StdToWeaponRot (Yaw -90°) 将标准基向量 (+X前, +Y右) 映射为武器/Sight 局部方向 (-Y前, +X右)
		const FQuat StdToWeaponRot = FRotator(0.f, -90.f, 0.f).Quaternion();
		const FQuat PureSightRotInCam = PureBoneInCam.GetRotation() * SightLocalToBone.GetRotation() * StdToWeaponRot;

		// 算出将 Sight 插槽回正对齐到摄像机中心轴所需的目标旋转偏移
		const FQuat NeededRotQuat = PureSightRotInCam.Inverse();
		CachedADSOffsetRot = NeededRotQuat.Rotator().GetNormalized();

		// 算出绕 weapon_root 施加该旋转后 Sight 插槽在 Camera 空间的位置，并求到达摄像机正前方所需的位移差
		const FVector RotatedSightOffsetInCam = (NeededRotQuat * PureBoneInCam.GetRotation()).RotateVector(SightLocalToBone.GetLocation());
		const FVector PureSightLocAfterRot = PureBoneInCam.GetLocation() + RotatedSightOffsetInCam;
		const FVector TargetSightLocInCam(FragWeapon->ADSEyeReliefDistance, 0.f, 0.f);

		CachedADSOffsetLoc = TargetSightLocInCam - PureSightLocAfterRot;
	}
}

FTransform UAnimInstHuman1P::BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FRotator& StdRotation)
{
	// 1. FPArms 网格体在角色蓝图中绕 Z 轴旋转了 -90 度，因此组件空间中：+Y = 前后(+前), -X = 左右(+右), +Z = 上下(+上)
	const FQuat StdToComp = FRotator(0.f, 90.f, 0.f).Quaternion();
	const FQuat StdQuat = StdRotation.Quaternion();
	const FQuat CompSpaceRot = StdToComp * StdQuat * StdToComp.Inverse();
	const FVector CompSpaceLoc = StdToComp.RotateVector(StdLocation);

	// 记录本帧实际写入 weapon_root 的组件空间旋转与位移，供下一帧 NativeUpdateAnimation 剥离
	LastAppliedCompRot = FQuat::Slerp(FQuat::Identity, CompSpaceRot, WeaponIKAlpha);
	LastAppliedCompLoc = FMath::Lerp(FVector::ZeroVector, CompSpaceLoc, WeaponIKAlpha);

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

USkeletalMeshComponent* UAnimInstHuman1P::GetTargetWeaponMesh(const AWeapon* Weapon) const
{
	return Weapon ? Weapon->EquipmentMesh1P : nullptr;
}
