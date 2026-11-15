#include "AnimInstHuman1P.h"

#include "HumanCharacter.h"
#include "Camera/CameraComponent.h"
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
	const int32 WeaponRootBoneIdx = SkelMesh ? SkelMesh->GetBoneIndex(Bones1P::WeaponRoot) : INDEX_NONE;
	if (WeaponRootBoneIdx == INDEX_NONE)
	{
		return;
	}

	// 1. 读取 weapon_root 在组件空间中的当前变换，并剥离上一帧 Control Rig 写回的附加旋转与位移，
	//    还原 Blender 纯动画本身赋予 weapon_root 的纯净基准姿态
	const FTransform RawBoneCompTransform = SkelMesh->GetBoneTransform(WeaponRootBoneIdx, FTransform::Identity);
	CachedWeaponRootCompRot = (LastAppliedCompRot.Inverse() * RawBoneCompTransform.GetRotation()).GetNormalized();
	const FVector PureWeaponRootCompLoc = RawBoneCompTransform.GetLocation() - LastAppliedCompLoc;

	// 2. 计算 1P 瞄准插槽 (Sight) 到摄像机正前方路径的精确偏移
	if (!HumanChar || !HumanChar->Camera || !CombatComp) return;
	AWeapon* CurWeapon = CombatComp->GetCurWeapon();
	if (!CurWeapon || !CurWeapon->FragWeapon || !CurWeapon->EquipmentMesh1P) return;

	if (CurWeapon->EquipmentMesh1P->DoesSocketExist(Sockets::Sight))
	{
		const UFragWeapon* FragWeapon = CurWeapon->FragWeapon;

		// 将纯净的 weapon_root 组件空间变换转为世界空间（若 WeaponRoot 插槽相对骨骼存在局部偏移则一并计入），再求相对于 Camera 的变换
		FTransform PureWeaponComp(CachedWeaponRootCompRot, PureWeaponRootCompLoc, FVector::OneVector);
		if (const USkeletalMeshSocket* WeaponRootSocket = SkelMesh->GetSocketByName(Sockets::WeaponRoot);
			WeaponRootSocket && WeaponRootSocket->BoneName == Bones1P::WeaponRoot)
		{
			PureWeaponComp = WeaponRootSocket->GetSocketLocalTransform() * PureWeaponComp;
		}
		const FTransform PureWeaponWorld = PureWeaponComp * SkelMesh->GetComponentTransform();
		const FTransform CamWorld = HumanChar->Camera->GetComponentTransform();
		const FTransform PureWeaponInCam = PureWeaponWorld.GetRelativeTransform(CamWorld);

		// 获取 Sight 插槽相对于武器网格体根部的局部变换
		const FTransform SightLocalToWeapon = CurWeapon->EquipmentMesh1P->GetSocketTransform(Sockets::Sight, RTS_Component);

		// 武器网格体局部空间为 -Y=前, +X=右, +Z=上，而标准/Camera坐标系为 +X=前, +Y=右, +Z=上：
		// 右乘 StdToWeaponRot (Yaw -90°) 将标准基向量 (+X前, +Y右) 映射为武器/Sight 局部方向 (-Y前, +X右)
		const FQuat StdToWeaponRot = FRotator(0.f, -90.f, 0.f).Quaternion();
		const FQuat TargetSightRotInCam = FragWeapon->ADSOffsetRotation.Quaternion();
		const FQuat PureSightRotInCam = PureWeaponInCam.GetRotation() * SightLocalToWeapon.GetRotation() * StdToWeaponRot;

		// 算出需要施加给武器的目标旋转偏移
		const FQuat NeededRotQuat = TargetSightRotInCam * PureSightRotInCam.Inverse();
		CachedADSOffsetRot = NeededRotQuat.Rotator().GetNormalized();

		// 算出施加该旋转后 Sight 插槽在 Camera 空间的位置，并求到达摄像机正前方所需的位移差
		const FVector RotatedSightOffsetInCam = (NeededRotQuat * PureWeaponInCam.GetRotation()).RotateVector(SightLocalToWeapon.GetLocation());
		const FVector PureSightLocAfterRot = PureWeaponInCam.GetLocation() + RotatedSightOffsetInCam;
		const FVector TargetSightLocInCam = FVector(FragWeapon->ADSEyeReliefDistance, 0.f, 0.f) + FragWeapon->ADSOffsetLocation;

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
