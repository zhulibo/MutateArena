#include "AnimInstHuman.h"

#include "HumanCharacter.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

void UAnimInstHuman::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 缓存当前武器的 3P 机瞄举枪偏移与歪头贴腮角度供工作线程安全读取
	if (CombatComp)
	{
		if (AWeapon* CurWeapon = CombatComp->GetCurWeapon(); CurWeapon && CurWeapon->FragWeapon)
		{
			// 3P 机瞄偏移：叠加在腰射右手基准骨骼 (VB spine_05_hand_r) 上的举枪增量偏移。
			// 标准坐标系下：向后抵肩(-X)、向左靠拢胸口中心(-Y)、向上抬起迎合右脸颊贴腮(+Z)，按 ADSScale 缩放。
			const FVector Base3PRaiseOffset(-2.f, -6.f, 6.f);
			CachedADSOffsetLoc = Base3PRaiseOffset * GetADSOffsetScale();
			CachedADSOffsetRot = FRotator::ZeroRotator;
			CachedADSHeadTiltRot = CurWeapon->FragWeapon->ADSHeadTiltRotation3P;
		}
		else
		{
			CachedADSHeadTiltRot = FRotator::ZeroRotator;
		}
	}

	// 读取 CR_Human 中 Ctrl_WeaponPivot 对齐的基准骨骼 VB spine_05_hand_r 的组件空间旋转
	// （CR_Human 仅通过 FABRIK 修改手部骨骼，不修改虚拟骨骼 VB spine_05_hand_r 本身，因此直接读取即为纯动画朝向）
	if (USkeletalMeshComponent* SkelMesh = GetSkelMeshComponent())
	{
		int32 BoneIdx = SkelMesh->GetBoneIndex(Bones::VBRightHand);
		if (BoneIdx == INDEX_NONE)
		{
			BoneIdx = SkelMesh->GetBoneIndex(Bones::RightHand);
		}
		if (BoneIdx != INDEX_NONE)
		{
			CachedHandBoneCompRot = SkelMesh->GetBoneQuaternion(SkelMesh->GetBoneName(BoneIdx), EBoneSpaces::ComponentSpace).GetNormalized();
		}
	}
}

void UAnimInstHuman::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (HumanChar)
	{
		// 先更新 AimYaw 与 AimPitch，确保后续 Super 调用 BuildWeaponPivotOffsetTransform 时使用当前帧最新俯仰角
		const FRotator AimRotation = HumanChar->GetBaseAimRotation();
		const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(HumanChar->GetVelocity());
		const FRotator DiffRotation = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
		AimYaw = DiffRotation.Yaw;
		AimPitch = HumanChar->AimPitch;
	}

	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// 计算 3P 瞄准歪头贴腮旋转（直接复用基类 SmoothAimAlpha，播放换弹/切枪蒙太奇时自动回正）
	const FRotator CurrentStdTilt = FMath::Lerp(FRotator::ZeroRotator, CachedADSHeadTiltRot, SmoothAimAlpha);

	// 转换到 MetaHuman BodyMesh 组件空间（绕 Z 轴 -90 度，且随 AimPitch 俯仰），确保无论角色抬头还是低头，歪头方向始终沿着视线轴向右侧倾
	const FQuat StdToComp = FRotator(0.f, 90.f, 0.f).Quaternion() * FRotator(AimPitch, 0.f, 0.f).Quaternion();
	const FQuat CompSpaceTiltQuat = StdToComp * CurrentStdTilt.Quaternion() * StdToComp.Inverse();

	// 按四元数球面插值拆分为 neck_01 (40%) 与 neck_02 (60%)，避免大俯仰角下欧拉角万向锁畸变
	Neck01ADSRotation = FQuat::Slerp(FQuat::Identity, CompSpaceTiltQuat, 0.4f).Rotator();
	Neck02ADSRotation = FQuat::Slerp(FQuat::Identity, CompSpaceTiltQuat, 0.6f).Rotator();
}

FTransform UAnimInstHuman::BuildWeaponPivotOffsetTransform(const FVector& StdLocation, const FRotator& StdRotation)
{
	// 1. MetaHuman BodyMesh 在角色蓝图中绕 Z 轴旋转了 -90 度（组件空间 +Y=前, -X=右, +Z=上），
	//    同时第三人称通过 AimOffset 驱动上半身俯仰，因此叠加 AimPitch 使后坐力与晃动始终沿着角色当前瞄准俯仰方向生效
	const FQuat StdToComp = FRotator(0.f, 90.f, 0.f).Quaternion() * FRotator(AimPitch, 0.f, 0.f).Quaternion();
	const FQuat StdQuat = StdRotation.Quaternion();

	// 2. 第三人称关卡序列动画中 hand_r 随不同枪械握持姿势旋转，而 CR_Human 中 Multiply(A, B) 会将 A 放在 B (VB spine_05_hand_r) 的局部空间下计算。
	//    乘以 CachedHandBoneCompRot.Inverse() 抵消手部骨骼旋转，使无论 hand_r 握枪角度如何、插槽如何旋转，
	//    UFragWeapon 的 X=前后, Y=左右, Z=上下 始终严格沿着角色瞄准的前后、左右、上下方向生效！
	const FQuat StdToBone = CachedHandBoneCompRot.Inverse() * StdToComp;
	const FVector BoneSpaceLoc = StdToBone.RotateVector(StdLocation);
	const FQuat BoneSpaceRot = StdToBone * StdQuat * StdToBone.Inverse();

	return FTransform(BoneSpaceRot, BoneSpaceLoc, FVector::OneVector);
}

USkeletalMeshComponent* UAnimInstHuman::GetTargetWeaponMesh(const AWeapon* Weapon) const
{
	return Weapon ? Weapon->EquipmentMesh : nullptr;
}
