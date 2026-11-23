#include "AnimInstHumanBase.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "HumanCharacter.h"
#include "Components/CombatComponent.h"
#include "Components/MAMovementComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

namespace
{
	// 6 轴二阶弹簧基准参数：不同固有频率天然形成“先向后撞肩(高频) -> 再枪口上扬(中频) -> 最后横向游走(低频)”的物理层次
	constexpr float FreqLocX  = 8.5f, DampLocX  = 0.50f;
	constexpr float FreqRoll  = 6.5f, DampRoll  = 0.35f;
	constexpr float FreqLocZ  = 5.0f, DampLocZ  = 0.55f;
	constexpr float FreqPitch = 4.5f, DampPitch = 0.62f;
	constexpr float FreqLocY  = 3.8f, DampLocY  = 0.65f;
	constexpr float FreqYaw   = 4.0f, DampYaw   = 0.70f;
	constexpr float RecoilImpulseVelScale = 4.f * PI; // 将单发目标位移/角度转换为二阶弹簧初速度的基准系数

	// 单自由度非对称二阶弹簧子步长积分求解器（去程使用 KickSpeedMul，回正使用 RecoverySpeedMul）
	void StepRecoilSpring(
		double& Current,
		double& Velocity,
		float DeltaTime,
		float Frequency,
		float DampingRatio,
		float KickSpeedMul,
		float RecoverySpeedMul,
		float DampingMul)
	{
		if (DeltaTime <= KINDA_SMALL_NUMBER || (Current == 0.0 && Velocity == 0.0))
		{
			return;
		}

		constexpr float MaxSubStep = 0.008f;
		const float ClampedDT = FMath::Min(DeltaTime, 0.033f);
		const int32 SubSteps = FMath::Max(1, FMath::CeilToInt(ClampedDT / MaxSubStep));
		const double SubDT = static_cast<double>(ClampedDT) / SubSteps;

		const double BaseFreq = static_cast<double>(FMath::Max(0.1f, Frequency));
		const double SafeKickMul = static_cast<double>(FMath::Max(0.1f, KickSpeedMul));
		const double SafeRecovMul = static_cast<double>(FMath::Max(0.1f, RecoverySpeedMul));
		const double EffDamping = static_cast<double>(FMath::Clamp(DampingRatio * DampingMul, 0.1f, 2.0f));

		for (int32 Step = 0; Step < SubSteps; ++Step)
		{
			const bool bIsKicking = (Current * Velocity > 0.0) || (FMath::Abs(Current) < 1e-4 && FMath::Abs(Velocity) > 1e-4);
			const double Omega = 2.0 * PI * BaseFreq * (bIsKicking ? SafeKickMul : SafeRecovMul);
			const double Accel = -(Omega * Omega) * Current - (2.0 * EffDamping * Omega) * Velocity;

			Velocity += Accel * SubDT;
			Current += Velocity * SubDT;
		}

		// 极小残余震荡直接归零，防止衰减末期浮点微震
		if (FMath::Abs(Current) < 1e-3 && FMath::Abs(Velocity) < 1e-2)
		{
			Current = 0.0;
			Velocity = 0.0;
		}
	}

	// 限制位置边界并在触壁外冲时清零该轴速度，防止弹簧在上限处粘滞
	void ClampRecoilAxis(double& Current, double& Velocity, double MinVal, double MaxVal)
	{
		if (Current < MinVal)
		{
			Current = MinVal;
			if (Velocity < 0.0) Velocity = 0.0;
		}
		else if (Current > MaxVal)
		{
			Current = MaxVal;
			if (Velocity > 0.0) Velocity = 0.0;
		}
	}
}

UAnimInstHumanBase::UAnimInstHumanBase()
{
}

void UAnimInstHumanBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	HumanChar = Cast<AHumanCharacter>(TryGetPawnOwner());
	if (HumanChar)
	{
		CombatComp = HumanChar->CombatComp;
		MovementComp = HumanChar->MovementComp;
	}
}

void UAnimInstHumanBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(TryGetPawnOwner());
	if (CombatComp == nullptr) CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	if (MovementComp == nullptr) MovementComp = HumanChar ? HumanChar->MovementComp : nullptr;
	if (!HumanChar || !CombatComp) return;

	// 在游戏主线程读取装备配置与蒙太奇状态，缓存供动画工作线程安全计算
	AEquipment* CurEquipment = CombatComp->GetCurEquipment();
	AWeapon* CurWeapon = CombatComp->GetCurWeapon();

	const EEquipmentName NewEquipName = CurEquipment ? CurEquipment->EquipmentName : EEquipmentName::None;
	if (EquipmentName != NewEquipName)
	{
		// 切换装备时清空残留的程序化瞄准与后坐力状态，防止上一把枪的过渡权重带入新武器的切入动画
		AimAlpha = 0.f;
		SmoothAimAlpha = 0.f;
		CachedADSOffsetLoc = FVector::ZeroVector;
		CachedADSOffsetRot = FRotator::ZeroRotator;

		CurrentRecoilLocation = FVector::ZeroVector;
		CurrentRecoilRotation = FRotator::ZeroRotator;
		RecoilLocVelocity = FVector::ZeroVector;
		RecoilRotVelocity = FRotator::ZeroRotator;
		RecoilShotCounter = 0;

		PumpElapsedTime = -1.f;
		PumpImpulseStage = 0;
		LeftHandOffsetLocation = FVector::ZeroVector;
		EquipmentName = NewEquipName;
	}

	bHasEquipment = (CurEquipment != nullptr && CurEquipment->EquipmentAsset != nullptr);
	bHasWeapon = (CurWeapon != nullptr && CurWeapon->FragWeapon != nullptr);
	bIsPlayingActionMontage = IsAnyMontagePlaying();
	bRawIsAiming = bHasWeapon && CombatComp->bIsAiming;

	const UAnimMontage* CurActiveMontage = GetCurrentActiveMontage();
	// 仅在新动作蒙太奇启动（如中途换弹或切枪）时打断拉泵，避免切枪/换弹尾段提前开火误杀第一枪拉泵
	if (CurActiveMontage != nullptr && CurActiveMontage != PrevActiveMontage.Get())
	{
		PumpElapsedTime = -1.f;
		PumpImpulseStage = 0;
	}
	PrevActiveMontage = CurActiveMontage;

	if (bHasEquipment)
	{
		const UEquipmentAsset* EquipAsset = CurEquipment->EquipmentAsset;
		WalkPlayRate = HumanChar->GetMaxWalkSpeed() / FMath::Max(1.f, HumanChar->Level1MaxWalkSpeed);
		WalkPlayRate *= bRawIsAiming ? EquipAsset->GetAimingWalkSpeedMul() : EquipAsset->GetWalkSpeedMul();
	}
	else
	{
		WalkPlayRate = 1.f;
	}

	bHasPumpMotion = false;
	CachedPumpPivotOffset = FVector::ZeroVector;

	if (bHasWeapon)
	{
		const UFragWeapon* FragWeapon = CurWeapon->FragWeapon;
		CachedADSTime = FMath::Max(0.01f, FragWeapon->ADSTime);

		// 缓存武器程序化后坐力参数供动画工作线程安全读取
		CachedRecoilMaxLoc = FVector(
			FMath::Max(0.0, FragWeapon->RecoilMaxLocation.X),
			FMath::Max(0.0, FragWeapon->RecoilMaxLocation.Y),
			FMath::Max(0.0, FragWeapon->RecoilMaxLocation.Z)
		);
		CachedRecoilMaxRot = FRotator(
			FMath::Max(0.0, FragWeapon->RecoilMaxRotation.Pitch),
			FMath::Max(0.0, FragWeapon->RecoilMaxRotation.Yaw),
			FMath::Max(0.0, FragWeapon->RecoilMaxRotation.Roll)
		);
		CachedRecoilKickSpeedMul = FMath::Max(0.1f, FragWeapon->RecoilKickSpeedMul);
		CachedRecoilRecoverySpeedMul = FMath::Max(0.1f, FragWeapon->RecoilRecoverySpeedMul);
		CachedRecoilDampingMul = FMath::Max(0.05f, FragWeapon->RecoilDampingMul);
		CachedADSRecoilScale = FMath::Max(0.f, FragWeapon->ADSRecoilScale);

		// 缓存泵动武器拉泵运动配置，并将武器局部空间护木向后行程 (+Y) 直接映射至 Ctrl_WeaponPivot 局部空间
		if (const FWeaponFirePartMotion* PumpMotion = FragWeapon->FindFirePartMotion(EWeaponMechBone::Pump);
			PumpMotion && PumpMotion->CycleDuration > 0.f)
		{
			bHasPumpMotion = true;
			CachedPumpMotion = *PumpMotion;

			const float PumpTravelDist = PumpMotion->MaxLocationOffset.Size();
			const USkeletalMeshComponent* CharSkelMesh = GetSkelMeshComponent();
			const USkeletalMeshComponent* TargetWeaponMesh = GetTargetWeaponMesh(CurWeapon);
			if (CharSkelMesh && TargetWeaponMesh && PumpTravelDist > KINDA_SMALL_NUMBER)
			{
				const USkeletalMeshSocket* AttachSocket = CharSkelMesh->GetSocketByName(TargetWeaponMesh->GetAttachSocketName());
				const FQuat SocketRot = AttachSocket ? AttachSocket->GetSocketLocalTransform().GetRotation() : FQuat::Identity;
				// 武器网格体约定 -Y=前, +Y=后；直接按护木行程距离沿武器 +Y 方向映射，避免骨骼局部轴向差异及污染 1P 状态
				const FVector WeaponLocalDelta = TargetWeaponMesh->GetRelativeScale3D() * FVector(0.f, PumpTravelDist, 0.f);
				CachedPumpPivotOffset = SocketRot.RotateVector(WeaponLocalDelta);
			}
		}
	}
	else
	{
		CachedADSTime = 0.2f;
		CachedADSOffsetLoc = FVector::ZeroVector;
		CachedADSOffsetRot = FRotator::ZeroRotator;
	}
}

void UAnimInstHumanBase::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!HumanChar || !CombatComp || !MovementComp) return;

	bIsOnLadder = MovementComp->MovementMode == MOVE_Custom && MovementComp->CustomMovementMode == CMOVE_Ladder;

	const FVector Velocity = HumanChar->GetVelocity();
	Speed = bIsOnLadder ? FMath::Abs(Velocity.Z) : Velocity.Size2D();

	bIsInAir = MovementComp->IsFalling() && !bIsOnLadder;
	bIsAccelerating = MovementComp->GetCurrentAcceleration().Size() > 0.f;
	bIsCrouched = HumanChar->bIsCrouched;

	// 1. 双臂 IK / Control Rig 权重：持握装备时启用
	const float TargetIKAlpha = bHasEquipment ? 1.f : 0.f;
	WeaponIKAlpha = FMath::FInterpTo(WeaponIKAlpha, TargetIKAlpha, DeltaSeconds, 15.f);

	// 2. 程序化瞄准偏移（播放动作蒙太奇时自动平滑退出瞄准偏移，采用单一对称缓动确保半途打断反转零跳变）
	bIsAiming = bRawIsAiming && !bIsPlayingActionMontage;
	const float AimStep = DeltaSeconds / FMath::Max(0.01f, CachedADSTime);
	AimAlpha = FMath::Clamp(AimAlpha + (bIsAiming ? AimStep : -AimStep), 0.f, 1.f);
	SmoothAimAlpha = FMath::InterpEaseInOut(0.f, 1.f, AimAlpha, 2.f);

	// 举枪半途包络 (0 -> 1 -> 0)：模拟举枪下抄弧线与手腕微内倾，腰射(0)与完全开镜(1)时严格为 0
	const float MidArc = FMath::Sin(AimAlpha * PI) * GetADSOffsetScale();
	const FVector CurrentADSLocation = CachedADSOffsetLoc * SmoothAimAlpha + FVector(0.f, 0.f, -0.5f * MidArc);
	const FRotator CurrentADSRotation = CachedADSOffsetRot * SmoothAimAlpha + FRotator(0.f, 0.f, -3.f * MidArc);

	// 3. 程序化呼吸、步伐摆动与移动惯性偏移
	if (bHasEquipment && !bIsPlayingActionMontage)
	{
		const float MaxRefSpeed = FMath::Max(1.f, HumanChar->Level1MaxWalkSpeed);
		const float SpeedRatio = !bIsInAir ? FMath::Clamp(Speed / MaxRefSpeed, 0.f, 1.f) : 0.f;

		const float RightSpeed = FVector::DotProduct(Velocity, HumanChar->GetActorRightVector());
		const float StrafeRatio = !bIsInAir ? FMath::Clamp(RightSpeed / MaxRefSpeed, -1.f, 1.f) : 0.f;

		// 角频率：静止时平稳呼吸 (1.8 rad/s)，移动时随步频加快 (8 rad/s)
		const float SwayFreq = FMath::Lerp(1.8f, 8.f, SpeedRatio);
		SwayPhase = FMath::Fmod(SwayPhase + DeltaSeconds * SwayFreq, 2.f * PI);

		// 周期性横置“8字”摇摆单侧半径（单位: cm | 完全机瞄时整体收紧至 15%）
		const float MotionScale = GetProceduralMotionScale() * FMath::Lerp(1.f, 0.15f, SmoothAimAlpha);
		const float LocFwdAmp  = FMath::Lerp(0.10f, 0.2f, SpeedRatio) * MotionScale;
		const float LocHorAmp  = FMath::Lerp(0.15f, 0.3f, SpeedRatio) * MotionScale;
		const float LocVertAmp = FMath::Lerp(0.20f, 0.4f, SpeedRatio) * MotionScale;

		// 移动时的持续惯性偏移
		const float MovePullbackX = -1.25f * SpeedRatio * MotionScale;
		const float MovePullbackZ = -0.25f * SpeedRatio * MotionScale;
		const float StrafeOffsetY = 0.65f * StrafeRatio * MotionScale;
		const float StrafeRoll    = 2.00f * StrafeRatio * MotionScale;

		const FVector TargetSwayLoc(
			MovePullbackX + FMath::Cos(SwayPhase * 2.f) * LocFwdAmp,
			StrafeOffsetY + FMath::Sin(SwayPhase) * LocHorAmp,
			MovePullbackZ + FMath::Sin(SwayPhase * 2.f) * LocVertAmp
		);

		constexpr float ArmSwingAnglePerCm = 0.6f;
		const FRotator TargetSwayRot = FRotator::MakeFromEuler(FVector(
			TargetSwayLoc.Y * 0.2f + StrafeRoll,
			TargetSwayLoc.Z * ArmSwingAnglePerCm,
			TargetSwayLoc.Y * ArmSwingAnglePerCm
		));

		CurrentSwayLocation = FMath::VInterpTo(CurrentSwayLocation, TargetSwayLoc, DeltaSeconds, 12.f);
		CurrentSwayRotation = FMath::RInterpTo(CurrentSwayRotation, TargetSwayRot, DeltaSeconds, 12.f);
	}
	else
	{
		CurrentSwayLocation = FMath::VInterpTo(CurrentSwayLocation, FVector::ZeroVector, DeltaSeconds, 16.f);
		CurrentSwayRotation = FMath::RInterpTo(CurrentSwayRotation, FRotator::ZeroRotator, DeltaSeconds, 16.f);
	}

	// 3.5 程序化拉泵：同步驱动左手护木偏移与枪身后拉/前推顿挫
	if (bHasPumpMotion && PumpElapsedTime >= 0.f)
	{
		PumpElapsedTime += DeltaSeconds;
		if (PumpElapsedTime <= CachedPumpMotion.GetTotalDuration())
		{
			const float Delay = CachedPumpMotion.GetSafeStartDelay();
			const float PushStartTime = Delay + CachedPumpMotion.GetSafeDuration() * CachedPumpMotion.GetHoldEndRatio();
			const float VelMul = CachedRecoilKickSpeedMul * RecoilImpulseVelScale * PumpImpulseScale;

			if (PumpImpulseStage == 0 && PumpElapsedTime >= Delay)
			{
				RecoilLocVelocity.X += -1.2f * FreqLocX * VelMul;
				RecoilRotVelocity.Roll += 2.0f * FreqRoll * VelMul;
				PumpImpulseStage = 1;
			}
			if (PumpImpulseStage == 1 && PumpElapsedTime >= PushStartTime)
			{
				RecoilLocVelocity.X += 0.9f * FreqLocX * VelMul;
				RecoilRotVelocity.Roll += -1.5f * FreqRoll * VelMul;
				PumpImpulseStage = 2;
			}

			LeftHandOffsetLocation = CachedPumpPivotOffset * CachedPumpMotion.EvaluatePhase(PumpElapsedTime);
		}
		else
		{
			PumpElapsedTime = -1.f;
			PumpImpulseStage = 0;
			LeftHandOffsetLocation = FVector::ZeroVector;
		}
	}
	else if (!LeftHandOffsetLocation.IsNearlyZero(1e-3f))
	{
		LeftHandOffsetLocation = FMath::VInterpTo(LeftHandOffsetLocation, FVector::ZeroVector, DeltaSeconds, 25.f);
	}
	else
	{
		LeftHandOffsetLocation = FVector::ZeroVector;
	}

	// 4. 程序化开火后坐力：6 自由度非对称二阶弹簧步进求解 + 安全上限截断
	StepRecoilSpring(CurrentRecoilLocation.X, RecoilLocVelocity.X, DeltaSeconds, FreqLocX, DampLocX, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilLocation.Y, RecoilLocVelocity.Y, DeltaSeconds, FreqLocY, DampLocY, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilLocation.Z, RecoilLocVelocity.Z, DeltaSeconds, FreqLocZ, DampLocZ, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Pitch, RecoilRotVelocity.Pitch, DeltaSeconds, FreqPitch, DampPitch, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Yaw, RecoilRotVelocity.Yaw, DeltaSeconds, FreqYaw, DampYaw, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Roll, RecoilRotVelocity.Roll, DeltaSeconds, FreqRoll, DampRoll, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);

	const double RecoilBoundScale = static_cast<double>(GetProceduralMotionScale() * FMath::Lerp(1.f, CachedADSRecoilScale, AimAlpha));
	const FVector ScaledMaxLoc = CachedRecoilMaxLoc * RecoilBoundScale;
	const FRotator ScaledMaxRot = CachedRecoilMaxRot * RecoilBoundScale;
	ClampRecoilAxis(CurrentRecoilLocation.X, RecoilLocVelocity.X, -ScaledMaxLoc.X, ScaledMaxLoc.X * 0.5);
	ClampRecoilAxis(CurrentRecoilLocation.Y, RecoilLocVelocity.Y, -ScaledMaxLoc.Y, ScaledMaxLoc.Y);
	ClampRecoilAxis(CurrentRecoilLocation.Z, RecoilLocVelocity.Z, -ScaledMaxLoc.Z, ScaledMaxLoc.Z);
	ClampRecoilAxis(CurrentRecoilRotation.Pitch, RecoilRotVelocity.Pitch, -ScaledMaxRot.Pitch, ScaledMaxRot.Pitch);
	ClampRecoilAxis(CurrentRecoilRotation.Yaw, RecoilRotVelocity.Yaw, -ScaledMaxRot.Yaw, ScaledMaxRot.Yaw);
	ClampRecoilAxis(CurrentRecoilRotation.Roll, RecoilRotVelocity.Roll, -ScaledMaxRot.Roll, ScaledMaxRot.Roll);

	// 5. 汇总标准坐标系偏移，交由子类转换到各自基准骨骼局部空间输出给 Ctrl_WeaponPivot
	const FVector TotalOffsetLocation = CurrentADSLocation + CurrentSwayLocation + CurrentRecoilLocation;
	const FRotator TotalOffsetRotation = CurrentADSRotation + CurrentSwayRotation + CurrentRecoilRotation;
	WeaponPivotOffsetTransform = BuildWeaponPivotOffsetTransform(TotalOffsetLocation, TotalOffsetRotation);
}

void UAnimInstHumanBase::AddRecoilImpulse(const UFragWeapon* FragWeapon)
{
	if (!FragWeapon) return;

	// 瞄准状态下按武器配置的 ADSRecoilScale 收紧后坐力，并乘以一/三人称各自的幅度系数
	const float AimScale = GetProceduralMotionScale() * FMath::Lerp(1.f, FMath::Max(0.f, FragWeapon->ADSRecoilScale), AimAlpha);
	const float NoiseScale = AimScale * FMath::Max(0.f, FragWeapon->RecoilNoiseScale);

	FVector KickLoc = FragWeapon->RecoilKickLocation * AimScale;
	FRotator KickRot = FragWeapon->RecoilKickRotation * AimScale;

	++RecoilShotCounter;
	const float AltSign = (RecoilShotCounter % 2 == 0) ? 1.f : -1.f;
	KickLoc.Y += FMath::FRandRange(-0.15f, 0.15f) * NoiseScale;
	KickRot.Yaw += (AltSign * 0.12f + FMath::FRandRange(-0.25f, 0.25f)) * NoiseScale;
	KickRot.Roll += (AltSign * 0.40f + FMath::FRandRange(-0.20f, 0.20f)) * NoiseScale;

	const float VelMul = FMath::Max(0.1f, FragWeapon->RecoilKickSpeedMul) * RecoilImpulseVelScale;
	RecoilLocVelocity.X += KickLoc.X * FreqLocX * VelMul;
	RecoilLocVelocity.Y += KickLoc.Y * FreqLocY * VelMul;
	RecoilLocVelocity.Z += KickLoc.Z * FreqLocZ * VelMul;

	RecoilRotVelocity.Pitch += KickRot.Pitch * FreqPitch * VelMul;
	RecoilRotVelocity.Yaw   += KickRot.Yaw   * FreqYaw   * VelMul;
	RecoilRotVelocity.Roll  += KickRot.Roll  * FreqRoll  * VelMul;

	if (const FWeaponFirePartMotion* PumpMotion = FragWeapon->FindFirePartMotion(EWeaponMechBone::Pump);
		PumpMotion && PumpMotion->CycleDuration > 0.f)
	{
		CachedPumpMotion = *PumpMotion;
		bHasPumpMotion = true;
		PumpElapsedTime = 0.f;
		PumpImpulseStage = 0;
		PumpImpulseScale = AimScale;
	}
}