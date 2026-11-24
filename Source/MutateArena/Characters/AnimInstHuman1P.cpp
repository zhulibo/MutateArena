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
	if (!SkelMesh)
	{
		return;
	}

	// 1. 直接读取虚拟骨骼 VB root_weapon_root 的组件空间变换（CR_Human1P 仅修改 weapon_root 与手臂骨骼，
	//    不触碰虚拟骨骼本身，因此直接读取即为 Blender 纯动画赋予 weapon_root 的纯净基准姿态，与 3P 架构完全一致）
	int32 PureBoneIdx = SkelMesh->GetBoneIndex(Bones::VBWeaponRoot);
	if (PureBoneIdx == INDEX_NONE)
	{
		PureBoneIdx = SkelMesh->GetBoneIndex(Bones::WeaponRoot);
	}
	if (PureBoneIdx == INDEX_NONE)
	{
		return;
	}

	const FTransform RawPureBoneComp = SkelMesh->GetBoneTransform(PureBoneIdx, FTransform::Identity);
	CachedWeaponRootCompRot = RawPureBoneComp.GetRotation().GetNormalized();

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
		const FTransform PureBoneComp(CachedWeaponRootCompRot, RawPureBoneComp.GetLocation(), FVector::OneVector);
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
		const FQuat NeededRotQuat = PureSightRotInCam.Inverse().GetNormalized();
		CachedADSOffsetQuat = NeededRotQuat;

		// 算出绕 weapon_root 施加该旋转后 Sight 插槽相对 weapon_root 在 Camera 空间的力臂，并求到达摄像机正前方所需的位移差
		const FVector RotatedSightOffsetInCam = (NeededRotQuat * PureBoneInCam.GetRotation()).RotateVector(SightLocalToBone.GetLocation());
		CachedADSPivotOffset = RotatedSightOffsetInCam;
		const FVector PureSightLocAfterRot = PureBoneInCam.GetLocation() + RotatedSightOffsetInCam;
		const FVector TargetSightLocInCam(FragWeapon->ADSEyeReliefDistance, 0.f, 0.f);

		CachedADSOffsetLoc = TargetSightLocInCam - PureSightLocAfterRot;
	}
	else
	{
		CachedADSOffsetLoc = FVector::ZeroVector;
		CachedADSOffsetQuat = FQuat::Identity;
		CachedADSPivotOffset = FVector::ZeroVector;
	}
}

FTransform UAnimInstHuman1P::BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FQuat& StdRotation) const
{
	// 1. FPArms 网格体在角色蓝图中绕 Z 轴旋转了 -90 度，因此组件空间中：+Y = 前后(+前), -X = 左右(+右), +Z = 上下(+上)
	const FQuat StdToComp = FRotator(0.f, 90.f, 0.f).Quaternion();

	// 2. Blender 动画中移动/旋转了 weapon_root，而 CR_Human1P 中 Multiply(A, B) 会将 A 放在 B (weapon_root) 的局部空间下计算：
	//    Result.Loc = B_Rot * A_Loc + B_Loc
	//    Result.Rot = B_Rot * A_Rot
	//    乘以 CachedWeaponRootCompRot.Inverse() 抵消 B_Rot，使无论 weapon_root 在 Blender 里如何旋转，
	//    UFragWeapon 的 X=前后, Y=左右, Z=上下 始终严格沿着第一人称镜头的前后、左右、上下方向生效！
	const FQuat StdToBone = CachedWeaponRootCompRot.Inverse() * StdToComp;
	const FVector BoneSpaceLoc = StdToBone.RotateVector(StdLocation);
	const FQuat BoneSpaceRot = StdToBone * StdRotation * StdToBone.Inverse();

	return FTransform(BoneSpaceRot, BoneSpaceLoc, FVector::OneVector);
}

USkeletalMeshComponent* UAnimInstHuman1P::GetTargetWeaponMesh(const AWeapon* Weapon) const
{
	return Weapon ? Weapon->EquipmentMesh1P : nullptr;
}
