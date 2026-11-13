#include "AnimInstHumanBase.h"

#include "HumanCharacter.h"
#include "Components/CombatComponent.h"
#include "Components/MAMovementComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

namespace
{
	// 6 轴二阶弹簧频率(Hz)与阻尼比(<1.0 产生过冲回弹)
	constexpr float FreqLocX  = 7.0f, DampLocX  = 0.50f; // 前后撞肩与复进过冲
	constexpr float FreqLocY  = 4.5f, DampLocY  = 0.60f; // 左右平移
	constexpr float FreqLocZ  = 5.2f, DampLocZ  = 0.55f; // 上下跳动
	constexpr float FreqPitch = 4.2f, DampPitch = 0.55f; // 枪口上扬（略慢于 X 轴，形成先撞肩后抬头）
	constexpr float FreqYaw   = 3.8f, DampYaw   = 0.60f; // 左右偏航
	constexpr float FreqRoll  = 5.8f, DampRoll  = 0.35f; // 枪身拧转（低阻尼保留停火余波）

	// 根据弹簧频率与阻尼比，精确计算使单发最大波峰达到 1.0 单位所需的初速度倍率（兼容欠阻尼、临界阻尼与过阻尼）
	float CalcUnitPeakImpulseScale(float Frequency, float DampingRatio)
	{
		const float Omega = 2.f * PI * FMath::Max(0.1f, Frequency);
		const float Zeta = FMath::Clamp(DampingRatio, 0.1f, 2.0f);

		float ExpPower = 1.f;
		if (Zeta < 0.999f)
		{
			const float SqrtTerm = FMath::Sqrt(1.f - Zeta * Zeta);
			ExpPower = (Zeta / SqrtTerm) * FMath::Acos(Zeta);
		}
		else if (Zeta > 1.001f)
		{
			const float SqrtTerm = FMath::Sqrt(Zeta * Zeta - 1.f);
			ExpPower = (Zeta / SqrtTerm) * FMath::Loge(Zeta + SqrtTerm);
		}

		return Omega * FMath::Exp(ExpPower);
	}

	// 单自由度非对称二阶弹簧子步长积分求解器（爆发去程使用 KickSpeedMul，回正阶段使用 RecoverySpeedMul）
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
		if (DeltaTime <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		constexpr float MaxSubStep = 0.01f;
		const float ClampedDT = FMath::Min(DeltaTime, 0.033f);
		const int32 SubSteps = FMath::Max(1, FMath::CeilToInt(ClampedDT / MaxSubStep));
		const double SubDT = static_cast<double>(ClampedDT) / SubSteps;

		const double BaseFreq = static_cast<double>(FMath::Max(0.1f, Frequency));
		const double SafeKickMul = static_cast<double>(FMath::Max(0.1f, KickSpeedMul));
		const double SafeRecovMul = static_cast<double>(FMath::Max(0.1f, RecoverySpeedMul));
		const double EffDamping = static_cast<double>(FMath::Clamp(DampingRatio * DampingMul, 0.1f, 2.0f));

		for (int32 Step = 0; Step < SubSteps; ++Step)
		{
			// 从原点出发初冲时 (Current == 0, Velocity != 0) 也视为爆发去程
			const bool bIsKicking = (Current * Velocity > 0.0) || (FMath::Abs(Current) < 1e-6 && FMath::Abs(Velocity) > 1e-6);
			const double EffFreq = BaseFreq * (bIsKicking ? SafeKickMul : SafeRecovMul);
			const double Omega = 2.0 * PI * EffFreq;
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

	// 在游戏主线程读取武器配置与蒙太奇状态，缓存供动画工作线程安全计算
	AEquipment* CurEquipment = CombatComp->GetCurEquipment();
	AWeapon* CurWeapon = CombatComp->GetCurWeapon();

	const EEquipmentName NewEquipName = CurEquipment ? CurEquipment->EquipmentName : EEquipmentName::None;
	if (EquipmentName != NewEquipName)
	{
		// 切换武器时清空残留的程序化后坐力状态
		CurrentRecoilLocation = FVector::ZeroVector;
		CurrentRecoilRotation = FRotator::ZeroRotator;
		RecoilLocVelocity = FVector::ZeroVector;
		RecoilRotVelocity = FRotator::ZeroRotator;
		RecoilShotCounter = 0;
		EquipmentName = NewEquipName;
	}

	bHasEquipment = (CurEquipment != nullptr && CurEquipment->EquipmentAsset != nullptr);
	bHasWeapon = (CurWeapon != nullptr && CurWeapon->FragWeapon != nullptr);
	bIsPlayingActionMontage = IsAnyMontagePlaying();
	bRawIsAiming = bHasWeapon && CombatComp->IsAiming();

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

	if (bHasWeapon)
	{
		const UFragWeapon* FragWeapon = CurWeapon->FragWeapon;
		const float ADSScale = GetADSOffsetScale();
		CachedADSTime = FMath::Max(0.01f, FragWeapon->ADSTime);
		CachedADSOffsetLoc = FragWeapon->ADSOffsetLocation * ADSScale;
		CachedADSOffsetRot = FragWeapon->ADSOffsetRotation * ADSScale;

		// 缓存武器程序化后坐力参数供工作线程安全读取
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
		CachedRecoilNoiseScale = FMath::Max(0.f, FragWeapon->RecoilNoiseScale);
	}
	else
	{
		// 未持枪时只需清空机瞄目标偏移，后坐力已在切枪时归零，无需重复重置缓存参数
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

	// 2. 程序化瞄准偏移（播放动作蒙太奇时自动平滑退出瞄准偏移）
	bIsAiming = bRawIsAiming && !bIsPlayingActionMontage;
	const float TargetAimAlpha = bIsAiming ? 1.f : 0.f;
	const float ADSInterpRate = CachedADSTime > KINDA_SMALL_NUMBER ? (1.f / CachedADSTime) : 5.f;
	AimAlpha = FMath::FInterpConstantTo(AimAlpha, TargetAimAlpha, DeltaSeconds, ADSInterpRate);

	const float SmoothAimAlpha = FMath::SmoothStep(0.f, 1.f, AimAlpha);
	CurrentADSLocation = FMath::Lerp(FVector::ZeroVector, CachedADSOffsetLoc, SmoothAimAlpha);
	CurrentADSRotation = FMath::Lerp(FRotator::ZeroRotator, CachedADSOffsetRot, SmoothAimAlpha);

	// 3. 程序化呼吸、步伐摆动与移动惯性偏移
	if (bHasEquipment && !bIsPlayingActionMontage)
	{
		const float MaxRefSpeed = FMath::Max(1.f, HumanChar->Level1MaxWalkSpeed);
		const float SpeedRatio = !bIsInAir ? FMath::Clamp(Speed / MaxRefSpeed, 0.f, 1.f) : 0.f;

		const float RightSpeed = FVector::DotProduct(Velocity, HumanChar->GetActorRightVector());
		const float StrafeRatio = !bIsInAir ? FMath::Clamp(RightSpeed / MaxRefSpeed, -1.f, 1.f) : 0.f;

		// 频率：静止时平稳呼吸 (1.6 rad/s)，移动时随步频加快 (7.2 rad/s)
		const float SwayFreq = FMath::Lerp(1.6f, 7.2f, SpeedRatio);
		SwayPhase = FMath::Fmod(SwayPhase + DeltaSeconds * SwayFreq, 2.f * PI);

		// 周期性“8字”摇摆幅度：瞄准时收紧至 15%
		const float MotionScale = GetProceduralMotionScale() * FMath::Lerp(1.f, 0.15f, SmoothAimAlpha);
		const float LocFwdAmp  = FMath::Lerp(0.10f, 0.25f, SpeedRatio) * MotionScale;
		const float LocHorAmp  = FMath::Lerp(0.25f, 0.75f, SpeedRatio) * MotionScale;
		const float LocVertAmp = FMath::Lerp(0.30f, 0.65f, SpeedRatio) * MotionScale;

		// 跑动后缩下沉与左右横移牵引
		const float MovePullbackX = -0.9f * SpeedRatio * MotionScale;
		const float MovePullbackZ = -0.25f * SpeedRatio * MotionScale;
		const float StrafeOffsetY = 0.65f * StrafeRatio * MotionScale;
		const float StrafeRoll    = 0.85f * StrafeRatio * MotionScale;

		const FVector TargetSwayLoc(
			MovePullbackX + FMath::Cos(SwayPhase * 2.f) * LocFwdAmp,
			StrafeOffsetY + FMath::Sin(SwayPhase) * LocHorAmp,
			MovePullbackZ + FMath::Sin(SwayPhase * 2.f) * LocVertAmp
		);

		// 顺应小臂摆动的微旋转 (X=Roll, Y=Pitch, Z=Yaw)
		constexpr float ArmSwingAnglePerCm = 0.6f;
		const FRotator TargetSwayRot = FRotator::MakeFromEuler(FVector(
			TargetSwayLoc.Y * 0.2f + StrafeRoll,
			TargetSwayLoc.Z * ArmSwingAnglePerCm,
			TargetSwayLoc.Y * ArmSwingAnglePerCm
		));

		CurrentSwayLocation = FMath::VInterpTo(CurrentSwayLocation, TargetSwayLoc, DeltaSeconds, 8.f);
		CurrentSwayRotation = FMath::RInterpTo(CurrentSwayRotation, TargetSwayRot, DeltaSeconds, 8.f);
	}
	else
	{
		CurrentSwayLocation = FMath::VInterpTo(CurrentSwayLocation, FVector::ZeroVector, DeltaSeconds, 12.f);
		CurrentSwayRotation = FMath::RInterpTo(CurrentSwayRotation, FRotator::ZeroRotator, DeltaSeconds, 12.f);
	}

	// 4. 程序化开火后坐力：6 自由度非对称二阶弹簧步进求解 + 安全上限截断
	StepRecoilSpring(CurrentRecoilLocation.X, RecoilLocVelocity.X, DeltaSeconds, FreqLocX, DampLocX, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilLocation.Y, RecoilLocVelocity.Y, DeltaSeconds, FreqLocY, DampLocY, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilLocation.Z, RecoilLocVelocity.Z, DeltaSeconds, FreqLocZ, DampLocZ, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Pitch, RecoilRotVelocity.Pitch, DeltaSeconds, FreqPitch, DampPitch, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Yaw, RecoilRotVelocity.Yaw, DeltaSeconds, FreqYaw, DampYaw, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Roll, RecoilRotVelocity.Roll, DeltaSeconds, FreqRoll, DampRoll, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);

	// 限制最大累积边界，并在触界外冲时清零对应速度防止粘滞（X 正向复进过冲上限为反向的一半）
	ClampRecoilAxis(CurrentRecoilLocation.X, RecoilLocVelocity.X, -CachedRecoilMaxLoc.X, CachedRecoilMaxLoc.X * 0.5);
	ClampRecoilAxis(CurrentRecoilLocation.Y, RecoilLocVelocity.Y, -CachedRecoilMaxLoc.Y, CachedRecoilMaxLoc.Y);
	ClampRecoilAxis(CurrentRecoilLocation.Z, RecoilLocVelocity.Z, -CachedRecoilMaxLoc.Z, CachedRecoilMaxLoc.Z);
	ClampRecoilAxis(CurrentRecoilRotation.Pitch, RecoilRotVelocity.Pitch, -CachedRecoilMaxRot.Pitch, CachedRecoilMaxRot.Pitch);
	ClampRecoilAxis(CurrentRecoilRotation.Yaw, RecoilRotVelocity.Yaw, -CachedRecoilMaxRot.Yaw, CachedRecoilMaxRot.Yaw);
	ClampRecoilAxis(CurrentRecoilRotation.Roll, RecoilRotVelocity.Roll, -CachedRecoilMaxRot.Roll, CachedRecoilMaxRot.Roll);

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

	// 推进连续一维柏林噪波游标，结合奇偶交替符号形成连发 S 形游走与轴向拧动
	RecoilNoiseTime += 0.37f;
	++RecoilShotCounter;
	const float NoiseA = FMath::PerlinNoise1D(RecoilNoiseTime);
	const float NoiseB = FMath::PerlinNoise1D(RecoilNoiseTime + 17.3f);
	const float AltSign = (RecoilShotCounter % 2 == 0) ? 1.f : -1.f;

	KickLoc.Y += (NoiseA * 0.18f + FMath::FRandRange(-0.04f, 0.04f)) * NoiseScale;
	KickRot.Yaw += (NoiseB * 0.32f + AltSign * 0.12f + FMath::FRandRange(-0.06f, 0.06f)) * NoiseScale;
	KickRot.Roll += (AltSign * 0.45f + NoiseA * 0.25f + FMath::FRandRange(-0.08f, 0.08f)) * NoiseScale;

	// 按武器实际生效的爆发频率与阻尼比转化为初速度冲量，保证单发波峰 1:1 匹配 RecoilKick 配置
	const float KickSpeedMul = FMath::Max(0.1f, FragWeapon->RecoilKickSpeedMul);
	const float DampingMul = FMath::Max(0.05f, FragWeapon->RecoilDampingMul);

	RecoilLocVelocity.X += KickLoc.X * CalcUnitPeakImpulseScale(FreqLocX * KickSpeedMul, FMath::Clamp(DampLocX * DampingMul, 0.1f, 2.0f));
	RecoilLocVelocity.Y += KickLoc.Y * CalcUnitPeakImpulseScale(FreqLocY * KickSpeedMul, FMath::Clamp(DampLocY * DampingMul, 0.1f, 2.0f));
	RecoilLocVelocity.Z += KickLoc.Z * CalcUnitPeakImpulseScale(FreqLocZ * KickSpeedMul, FMath::Clamp(DampLocZ * DampingMul, 0.1f, 2.0f));

	RecoilRotVelocity.Pitch += KickRot.Pitch * CalcUnitPeakImpulseScale(FreqPitch * KickSpeedMul, FMath::Clamp(DampPitch * DampingMul, 0.1f, 2.0f));
	RecoilRotVelocity.Yaw   += KickRot.Yaw   * CalcUnitPeakImpulseScale(FreqYaw   * KickSpeedMul, FMath::Clamp(DampYaw   * DampingMul, 0.1f, 2.0f));
	RecoilRotVelocity.Roll  += KickRot.Roll  * CalcUnitPeakImpulseScale(FreqRoll  * KickSpeedMul, FMath::Clamp(DampRoll  * DampingMul, 0.1f, 2.0f));
}