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
	// 6 轴二阶弹簧基准参数：
	// - Freq (Hz): 固有频率（决定达峰与回正快慢）
	// - Damp: 阻尼比（<1.0 为欠阻尼，产生过冲与余波回弹）
	// - Lag (秒): 冲量传导滞后时间常数（0 = 膛压瞬发；>0 = 抵肩/肌肉传导滞后，形成“先向后撞肩 -> 再枪口上扬 -> 最后横向游走”的物理鞭打层次）
	constexpr float FreqLocX  = 8.5f, DampLocX  = 0.50f, LagLocX  = 0.000f; // 1. 瞬发：膛压向后直线撞肩（最高频、零滞后）
	constexpr float FreqRoll  = 6.5f, DampRoll  = 0.35f, LagRoll  = 0.004f; // 1. 瞬发：弹头啮合膛线产生的轴向反扭矩（低阻尼保留停火后手腕余颤）
	constexpr float FreqLocZ  = 5.0f, DampLocZ  = 0.55f, LagLocZ  = 0.015f; // 2. 次级滞后 (15ms)：抵肩支点反作用力带动枪身垂直上跳
	constexpr float FreqPitch = 4.5f, DampPitch = 0.62f, LagPitch = 0.018f; // 2. 次级滞后 (18ms)：撞肩到位后力矩转化为枪口猛烈上扬
	constexpr float FreqLocY  = 3.8f, DampLocY  = 0.65f, LagLocY  = 0.025f; // 3. 末级滞后 (25ms)：双臂受激后的水平横向游走
	constexpr float FreqYaw   = 4.0f, DampYaw   = 0.70f, LagYaw   = 0.028f; // 3. 末级滞后 (28ms)：手腕卸力产生的枪口左右偏摆

	// 根据武器射速 FireRate (RPM) 自适应计算弹簧频率缩放因子（以 600 RPM / 10 RPS 为 1.0x 基准）
	float CalcFireRateFreqScale(float FireRateRPM)
	{
		if (FireRateRPM <= 0.f)
		{
			return 1.f;
		}

		// 项目内 FireRate 统一为 RPM（见 AWeapon::GetFireDelay），直接使用 /60 避免低射速狙击枪误判
		const float ShotsPerSec = FireRateRPM / 60.f;
		return FMath::Clamp(FMath::Pow(ShotsPerSec / 10.f, 0.35f), 0.70f, 1.35f);
	}

	// 根据弹簧频率、阻尼比与冲量滞后时间，精确计算使单发脉冲最大波峰达到 1.0 单位所需的冲量倍率
	float CalcUnitPeakImpulseScale(float Frequency, float DampingRatio, float ImpulseLag)
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

		// 补偿一阶滞后滤波 (Lag) 带来的波峰衰减，确保无论滞后多大，单发最大位移严格 1:1 匹配配置值
		const float OmegaLag = Omega * FMath::Max(0.f, ImpulseLag);
		const float LagAttenuationComp = FMath::Sqrt(1.f + OmegaLag * OmegaLag);

		return Omega * FMath::Exp(ExpPower) * LagAttenuationComp;
	}

	// 单自由度非对称二阶弹簧子步长积分求解器（支持 PendingImpulse 一阶滞后释放，去程使用 KickSpeedMul，回正使用 RecoverySpeedMul）
	void StepRecoilSpring(
		double& Current,
		double& Velocity,
		double& PendingImpulse,
		float DeltaTime,
		float Frequency,
		float DampingRatio,
		float ImpulseLag,
		float KickSpeedMul,
		float RecoverySpeedMul,
		float DampingMul)
	{
		if (DeltaTime <= KINDA_SMALL_NUMBER || (Current == 0.0 && Velocity == 0.0 && PendingImpulse == 0.0))
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
		const double EffLag = static_cast<double>(ImpulseLag) / SafeKickMul;

		for (int32 Step = 0; Step < SubSteps; ++Step)
		{
			// 1. 从滞后缓冲池向弹簧速度平滑释放冲量
			if (FMath::Abs(PendingImpulse) > 1e-6)
			{
				if (EffLag <= 1e-4)
				{
					Velocity += PendingImpulse;
					PendingImpulse = 0.0;
				}
				else
				{
					const double ReleaseAlpha = 1.0 - FMath::Exp(-SubDT / EffLag);
					const double Released = PendingImpulse * ReleaseAlpha;
					Velocity += Released;
					PendingImpulse -= Released;
				}
			}

			// 2. 位移与速度同号（正远离平衡点）、从平衡点刚起步、或缓冲池仍在剧烈注入冲量时，视为爆发去程
			const bool bIsKicking = (Current * Velocity > 0.0)
				|| (FMath::Abs(Current) < 1e-4 && FMath::Abs(Velocity) > 1e-4)
				|| (FMath::Abs(PendingImpulse) > FMath::Abs(Velocity) * 0.25);

			const double EffFreq = BaseFreq * (bIsKicking ? SafeKickMul : SafeRecovMul);
			const double Omega = 2.0 * PI * EffFreq;

			const double Accel = -(Omega * Omega) * Current - (2.0 * EffDamping * Omega) * Velocity;

			Velocity += Accel * SubDT;
			Current += Velocity * SubDT;
		}

		// 极小残余震荡直接归零，防止衰减末期浮点微震
		if (FMath::Abs(Current) < 1e-3 && FMath::Abs(Velocity) < 1e-2 && FMath::Abs(PendingImpulse) < 1e-3)
		{
			Current = 0.0;
			Velocity = 0.0;
			PendingImpulse = 0.0;
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
		bPrevAiming = false;
		AimAlpha = 0.f;
		SmoothAimAlpha = 0.f;
		AimSegmentStartSmooth = 0.f;
		AimSegmentStartLinear = 0.f;
		AimSegmentProgress = 1.f;
		AimSegmentDuration = 0.f;
		CurrentADSLocation = FVector::ZeroVector;
		CurrentADSRotation = FRotator::ZeroRotator;
		CachedADSOffsetLoc = FVector::ZeroVector;
		CachedADSOffsetRot = FRotator::ZeroRotator;

		CurrentRecoilLocation = FVector::ZeroVector;
		CurrentRecoilRotation = FRotator::ZeroRotator;
		RecoilLocVelocity = FVector::ZeroVector;
		RecoilRotVelocity = FRotator::ZeroRotator;
		PendingRecoilLocImpulse = FVector::ZeroVector;
		PendingRecoilRotImpulse = FRotator::ZeroRotator;
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
		const float FireRateScale = CalcFireRateFreqScale(FragWeapon->FireRate);
		CachedRecoilKickSpeedMul = FMath::Max(0.1f, FragWeapon->RecoilKickSpeedMul) * FireRateScale;
		CachedRecoilRecoverySpeedMul = FMath::Max(0.1f, FragWeapon->RecoilRecoverySpeedMul) * FireRateScale;
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
		// 未持枪时重置机瞄目标偏移（后坐力状态已在切换装备时归零）
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

	// 瞄准状态反转时，锁定当前 SmoothAimAlpha 与 AimAlpha 作为新分段起点，并按 SmoothAimAlpha 的实际剩余视觉行程折算耗时，
	// 防止在 EaseOut(开镜) 与 EaseInOut(收枪) 两条非对称缓动曲线间半途切换时产生单帧位置跳变或速度失真
	if (bIsAiming != bPrevAiming)
	{
		bPrevAiming = bIsAiming;
		AimSegmentStartSmooth = SmoothAimAlpha;
		AimSegmentStartLinear = AimAlpha;
		AimSegmentProgress = 0.f;
		AimSegmentDuration = CachedADSTime * FMath::Abs(TargetAimAlpha - AimSegmentStartSmooth);
	}

	AimSegmentProgress = AimSegmentDuration > KINDA_SMALL_NUMBER
		? FMath::Min(1.f, AimSegmentProgress + DeltaSeconds / AimSegmentDuration)
		: 1.f;

	AimAlpha = FMath::Lerp(AimSegmentStartLinear, TargetAimAlpha, AimSegmentProgress);

	// 开镜采用急起缓停 (EaseOut 2.6) 提升按键瞬间跟手度，收枪采用平滑加减速 (EaseInOut 2.0)
	const float CurveAlpha = bIsAiming
		? FMath::InterpEaseOut(0.f, 1.f, AimSegmentProgress, 2.6f)
		: FMath::InterpEaseInOut(0.f, 1.f, AimSegmentProgress, 2.0f);
	SmoothAimAlpha = FMath::Lerp(AimSegmentStartSmooth, TargetAimAlpha, CurveAlpha);

	const float ADSScale = GetADSOffsetScale();

	// (a) 举枪半途包络 (0 -> 1 -> 0)：模拟人手举枪“先内收再上托”的下抄弧线与手腕内倾，腰射(0)与完全开镜(1)时严格为 0
	const float MidArc = FMath::Sin(AimAlpha * PI) * (1.f - AimAlpha * 0.4f);
	// 按实际垂直抬升高度的 10% 自适应计算下抄弧线深度 (上限 0.8cm)，避免高度差极小时起步反向下掉
	const float ArcDipZ = FMath::Clamp(CachedADSOffsetLoc.Z * 0.1f, 0.f, 0.8f * ADSScale);
	// 按实际横向提拉距离自适应缩放手腕内倾幅度 (以 6cm 为满额基准)
	const float LateralRatio = FMath::Clamp(FMath::Abs(CachedADSOffsetLoc.Y) / FMath::Max(1.f, 6.f * ADSScale), 0.2f, 1.f);

	// (b) 开镜到位微过冲包络：仅在开镜后半程 (AimAlpha 0.5 -> 1.0) 产生正值波峰模拟抵肩与回正缓冲，分段起步前 25% 进度内通过 SettleBlendIn 平滑淡入
	const float SettleBlendIn = FMath::Clamp(AimSegmentProgress * 4.f, 0.f, 1.f);
	const float SettleWave = bIsAiming
		? (-FMath::Sin(AimAlpha * 2.f * PI) * FMath::Square(AimAlpha) * ADSScale * SettleBlendIn)
		: 0.f;

	// 合成附加动态位置：仅保留到位顺视线向后撞肩 (-X) 与半途托举下抄弧线 (-Z)，去除 Y/Z 末端过冲以确保准星到位瞬间稳固锁死
	const FVector DynamicADSLoc(
		-0.45f * SettleWave, // 到位瞬间枪托向后抵肩 (-X)，顺沿视线轴不改变屏幕中心准星位置
		0.f,                 // 零左右过冲，防止准星水平晃动
		-ArcDipZ * MidArc    // 举枪半途托举下抄弧线 (-Z)
	);

	// 合成附加动态旋转：仅保留 Roll 轴（半途手腕向左内倾 -3.2° + 到位瞬间向右微过冲回正 +0.65°），去除 Pitch/Yaw 防止长枪管放大准星跳动
	const FRotator DynamicADSRot = FRotator::MakeFromEuler(FVector(
		-3.2f * LateralRatio * MidArc + 0.65f * SettleWave, // X (Roll)
		0.f,                                                // Y (Pitch)
		0.f                                                 // Z (Yaw)
	));

	CurrentADSLocation = FMath::Lerp(FVector::ZeroVector, CachedADSOffsetLoc, SmoothAimAlpha) + DynamicADSLoc;
	CurrentADSRotation = FMath::Lerp(FRotator::ZeroRotator, CachedADSOffsetRot, SmoothAimAlpha) + DynamicADSRot;

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

		// 周期性横置“8字”摇摆单侧半径（单位: cm | 总峰峰值行程为单侧半径的 2 倍；完全机瞄时整体收紧至 15%）
		const float MotionScale = GetProceduralMotionScale() * FMath::Lerp(1.f, 0.15f, SmoothAimAlpha);
		const float LocFwdAmp  = FMath::Lerp(0.10f, 0.2f, SpeedRatio) * MotionScale; // 前后单侧摆幅 (X)
		const float LocHorAmp  = FMath::Lerp(0.15f, 0.3f, SpeedRatio) * MotionScale; // 左右单侧摆幅 (Y，8字半宽)
		const float LocVertAmp = FMath::Lerp(0.20f, 0.4f, SpeedRatio) * MotionScale; // 上下单侧摆幅 (Z，8字半高)

		// 移动时的持续惯性偏移（只要处于跑动或横移状态就会保持的基础位移与倾斜）
		const float MovePullbackX = -1.25f * SpeedRatio * MotionScale;  // 跑动时枪身整体向后收 (-X，单位: cm)
		const float MovePullbackZ = -0.25f * SpeedRatio * MotionScale; // 跑动时枪身整体向下沉 (-Z，单位: cm)
		const float StrafeOffsetY = 0.65f * StrafeRatio * MotionScale; // A/D 左右横移时，枪向同侧平移 (Y，单位: cm)
		const float StrafeRoll    = 2.00f * StrafeRatio * MotionScale; // A/D 左右横移时，枪身向同侧倾斜 (Roll，单位: 度)

		// 合成目标摇摆位置：左右使用 1 倍频 (Sin)，前后/上下使用 2 倍频 (2*SwayPhase)，走出横躺的“∞”字李萨如步伐轨迹
		const FVector TargetSwayLoc(
			MovePullbackX + FMath::Cos(SwayPhase * 2.f) * LocFwdAmp,
			StrafeOffsetY + FMath::Sin(SwayPhase) * LocHorAmp,
			MovePullbackZ + FMath::Sin(SwayPhase * 2.f) * LocVertAmp
		);

		// 随位置摆动附带的枪口微旋转（单位: 度 | Pitch/Yaw 随位移每 cm 顺势微转 0.6°，Roll 随横向位移每 cm 微倾 0.2° 并叠加横移侧倾）
		constexpr float ArmSwingAnglePerCm = 0.6f;
		const FRotator TargetSwayRot = FRotator::MakeFromEuler(FVector(
			TargetSwayLoc.Y * 0.2f + StrafeRoll,  // X (Roll 侧倾)：随左右位移微倾 + 横移侧倾角
			TargetSwayLoc.Z * ArmSwingAnglePerCm, // Y (Pitch 俯仰)：随上下颠簸微幅点头
			TargetSwayLoc.Y * ArmSwingAnglePerCm  // Z (Yaw 偏航)：随左右摆动微幅摆头
		));

		CurrentSwayLocation = FMath::VInterpTo(CurrentSwayLocation, TargetSwayLoc, DeltaSeconds, 12.f);
		CurrentSwayRotation = FMath::RInterpTo(CurrentSwayRotation, TargetSwayRot, DeltaSeconds, 12.f);
	}
	else
	{
		CurrentSwayLocation = FMath::VInterpTo(CurrentSwayLocation, FVector::ZeroVector, DeltaSeconds, 16.f);
		CurrentSwayRotation = FMath::RInterpTo(CurrentSwayRotation, FRotator::ZeroRotator, DeltaSeconds, 16.f);
	}

	// 3.5 程序化拉泵：同步驱动左手护木偏移与枪身后拉/前推次级后坐力顿挫
	if (bHasPumpMotion && PumpElapsedTime >= 0.f)
	{
		PumpElapsedTime += DeltaSeconds;
		if (PumpElapsedTime <= CachedPumpMotion.GetTotalDuration())
		{
			const float Delay = CachedPumpMotion.GetSafeStartDelay();
			const float Duration = CachedPumpMotion.GetSafeDuration();
			const float PushStartTime = Delay + Duration * CachedPumpMotion.GetHoldEndRatio();

			const float PumpLocScale = CalcUnitPeakImpulseScale(
				FreqLocX * CachedRecoilKickSpeedMul,
				FMath::Clamp(DampLocX * CachedRecoilDampingMul, 0.1f, 2.0f),
				LagLocX / CachedRecoilKickSpeedMul);
			const float PumpRotScale = CalcUnitPeakImpulseScale(
				FreqRoll * CachedRecoilKickSpeedMul,
				FMath::Clamp(DampRoll * CachedRecoilDampingMul, 0.1f, 2.0f),
				LagRoll / CachedRecoilKickSpeedMul);

			if (PumpImpulseStage == 0 && PumpElapsedTime >= Delay)
			{
				// 后拉开始：枪身轻微后坐与侧倾
				PendingRecoilLocImpulse.X += (-1.2f * PumpImpulseScale) * PumpLocScale;
				PendingRecoilRotImpulse.Roll += (2.0f * PumpImpulseScale) * PumpRotScale;
				PumpImpulseStage = 1;
			}
			if (PumpImpulseStage == 1 && PumpElapsedTime >= PushStartTime)
			{
				// 前推上膛：枪身轻微前顿与反向回正
				PendingRecoilLocImpulse.X += (0.9f * PumpImpulseScale) * PumpLocScale;
				PendingRecoilRotImpulse.Roll += (-1.5f * PumpImpulseScale) * PumpRotScale;
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
		// 被换弹等动作蒙太奇中途打断时平滑归零，避免左手瞬间弹回
		LeftHandOffsetLocation = FMath::VInterpTo(LeftHandOffsetLocation, FVector::ZeroVector, DeltaSeconds, 25.f);
	}
	else
	{
		LeftHandOffsetLocation = FVector::ZeroVector;
	}

	// 4. 程序化开火后坐力：6 自由度非对称二阶弹簧步进求解 + 安全上限截断
	StepRecoilSpring(CurrentRecoilLocation.X, RecoilLocVelocity.X, PendingRecoilLocImpulse.X, DeltaSeconds, FreqLocX, DampLocX, LagLocX, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilLocation.Y, RecoilLocVelocity.Y, PendingRecoilLocImpulse.Y, DeltaSeconds, FreqLocY, DampLocY, LagLocY, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilLocation.Z, RecoilLocVelocity.Z, PendingRecoilLocImpulse.Z, DeltaSeconds, FreqLocZ, DampLocZ, LagLocZ, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Pitch, RecoilRotVelocity.Pitch, PendingRecoilRotImpulse.Pitch, DeltaSeconds, FreqPitch, DampPitch, LagPitch, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Yaw, RecoilRotVelocity.Yaw, PendingRecoilRotImpulse.Yaw, DeltaSeconds, FreqYaw, DampYaw, LagYaw, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);
	StepRecoilSpring(CurrentRecoilRotation.Roll, RecoilRotVelocity.Roll, PendingRecoilRotImpulse.Roll, DeltaSeconds, FreqRoll, DampRoll, LagRoll, CachedRecoilKickSpeedMul, CachedRecoilRecoverySpeedMul, CachedRecoilDampingMul);

	// 限制最大累积边界（随一/三人称幅度系数与机瞄收紧倍率同步缩放），并在触界外冲时清零对应速度防止粘滞（X 轴向前复进过冲上限限制为向后撞肩上限的一半）
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

	// 推进连续一维柏林噪波游标，结合奇偶发交替符号形成连发 S 形水平游走与枪身轴向拧动
	RecoilNoiseTime += 0.37f;
	++RecoilShotCounter;
	const float NoiseA = FMath::PerlinNoise1D(RecoilNoiseTime);
	const float NoiseB = FMath::PerlinNoise1D(RecoilNoiseTime + 17.3f);
	const float AltSign = (RecoilShotCounter % 2 == 0) ? 1.f : -1.f;

	KickLoc.Y += (NoiseA * 0.18f + FMath::FRandRange(-0.04f, 0.04f)) * NoiseScale;
	KickRot.Yaw += (NoiseB * 0.32f + AltSign * 0.12f + FMath::FRandRange(-0.06f, 0.06f)) * NoiseScale;
	KickRot.Roll += (AltSign * 0.45f + NoiseA * 0.25f + FMath::FRandRange(-0.08f, 0.08f)) * NoiseScale;

	// 按武器实际生效的爆发频率（含射速自适应缩放）、阻尼比与轴向滞后时间转化为缓冲冲量，保证单发最大波峰 1:1 精确匹配 RecoilKick 配置
	const float FireRateScale = CalcFireRateFreqScale(FragWeapon->FireRate);
	const float KickSpeedMul = FMath::Max(0.1f, FragWeapon->RecoilKickSpeedMul) * FireRateScale;
	const float DampingMul = FMath::Max(0.05f, FragWeapon->RecoilDampingMul);

	PendingRecoilLocImpulse.X += KickLoc.X * CalcUnitPeakImpulseScale(FreqLocX * KickSpeedMul, FMath::Clamp(DampLocX * DampingMul, 0.1f, 2.0f), LagLocX / KickSpeedMul);
	PendingRecoilLocImpulse.Y += KickLoc.Y * CalcUnitPeakImpulseScale(FreqLocY * KickSpeedMul, FMath::Clamp(DampLocY * DampingMul, 0.1f, 2.0f), LagLocY / KickSpeedMul);
	PendingRecoilLocImpulse.Z += KickLoc.Z * CalcUnitPeakImpulseScale(FreqLocZ * KickSpeedMul, FMath::Clamp(DampLocZ * DampingMul, 0.1f, 2.0f), LagLocZ / KickSpeedMul);

	PendingRecoilRotImpulse.Pitch += KickRot.Pitch * CalcUnitPeakImpulseScale(FreqPitch * KickSpeedMul, FMath::Clamp(DampPitch * DampingMul, 0.1f, 2.0f), LagPitch / KickSpeedMul);
	PendingRecoilRotImpulse.Yaw   += KickRot.Yaw   * CalcUnitPeakImpulseScale(FreqYaw   * KickSpeedMul, FMath::Clamp(DampYaw   * DampingMul, 0.1f, 2.0f), LagYaw   / KickSpeedMul);
	PendingRecoilRotImpulse.Roll  += KickRot.Roll  * CalcUnitPeakImpulseScale(FreqRoll  * KickSpeedMul, FMath::Clamp(DampRoll  * DampingMul, 0.1f, 2.0f), LagRoll  / KickSpeedMul);

	// 若当前武器配置了有效的 Pump 往复运动，同步启动程序化拉泵计时器与次级顿挫冲量缩放
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