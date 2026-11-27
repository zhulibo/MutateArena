#include "RecoilComponent.h"

#include "CombatComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "TimerManager.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "MutateArena/Equipments/Weapon.h"

URecoilComponent::URecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URecoilComponent::BeginPlay()
{
	Super::BeginPlay();
	
	HumanCharacter = Cast<AHumanCharacter>(GetOwner());
}

void URecoilComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		AWeapon* CurWeapon = HumanCharacter->CombatComp ? HumanCharacter->CombatComp->GetCurWeapon() : nullptr;
		if (ActiveWeapon.IsValid() && CurWeapon != ActiveWeapon.Get())
		{
			// 切刀、切枪或丢弃武器时清空后坐力，防止切回枪械后镜头被延迟拉扯
			ResetRecoil();
			return;
		}

		if (bIsApplyingRecoil)
		{
			PollApplyRecoil(DeltaSeconds);
		}

		if (bIsWaitingRecover)
		{
			TryStartRecoverRecoil();
		}

		if (bIsRecoveringRecoil)
		{
			PollRecoverRecoil(DeltaSeconds);
		}

		PollDecayShotCount(DeltaSeconds);
	}
}

void URecoilComponent::ResetRecoil()
{
	ActiveWeapon.Reset();
	bIsApplyingRecoil = false;
	bIsWaitingRecover = false;
	bIsRecoveringRecoil = false;

	RecoilVertCurFire = 0.f;
	RecoilHorCurFire = 0.f;
	RecoilVertLastTick = 0.f;
	RecoilHorLastTick = 0.f;
	RecoilIncCostTime = 0.f;

	RecoilVertTotal = 0.f;
	RecoilHorTotal = 0.f;
	CameraRecoilVertTotal = 0.f;
	CameraRecoilHorTotal = 0.f;

	RecoilDecTime = 0.f;
	RecoilDecCostTime = 0.f;
	RecoilVertRecoverTotal = 0.f;
	RecoilHorRecoverTotal = 0.f;
	RecoilVertRecoverLastTick = 0.f;
	RecoilHorRecoverLastTick = 0.f;
	CameraRecoilVertRecoverTotal = 0.f;
	CameraRecoilHorRecoverTotal = 0.f;
	CameraRecoilVertRecoverLastTick = 0.f;
	CameraRecoilHorRecoverLastTick = 0.f;

	RecoilShotIndex = 1.f;
	RecoilShotDecayRate = 0.f;
	LastRecoilFireTime = -100.0;

	if (HumanCharacter && HumanCharacter->CombatComp)
	{
		HumanCharacter->CombatComp->CurShotCount = 1;
	}
}

int32 URecoilComponent::GetCurrentShotCount() const
{
	return FMath::Max(1, FMath::RoundToInt(RecoilShotIndex));
}

void URecoilComponent::OnControllerLookInput(float YawInput, float PitchInput)
{
	// 垂直压枪抵消：镜头上跳记为正值，玩家下拉鼠标（PitchInput > 0）时减少待恢复量
	if (CameraRecoilVertTotal > 0.f && PitchInput > 0.f)
	{
		CameraRecoilVertTotal = FMath::Max(0.f, CameraRecoilVertTotal - PitchInput);
	}
	else if (CameraRecoilVertTotal < 0.f && PitchInput < 0.f)
	{
		CameraRecoilVertTotal = FMath::Min(0.f, CameraRecoilVertTotal - PitchInput);
	}

	// 水平压枪抵消：玩家向相反方向拉鼠标时减少待恢复量
	if (CameraRecoilHorTotal > 0.f && YawInput < 0.f)
	{
		CameraRecoilHorTotal = FMath::Max(0.f, CameraRecoilHorTotal + YawInput);
	}
	else if (CameraRecoilHorTotal < 0.f && YawInput > 0.f)
	{
		CameraRecoilHorTotal = FMath::Min(0.f, CameraRecoilHorTotal + YawInput);
	}
}

// 开火后增加后坐力（在算完本发子弹落点后调用）
void URecoilComponent::IncRecoil(int32 ShotCount)
{
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr) return;

	AWeapon* Weapon = HumanCharacter->CombatComp->GetCurWeapon();
	if (Weapon == nullptr || Weapon->FragWeapon == nullptr) return;

	if (ActiveWeapon.IsValid() && ActiveWeapon.Get() != Weapon)
	{
		ResetRecoil();
	}
	ActiveWeapon = Weapon;

	const int32 EffectiveShot = FMath::Max(1, ShotCount);
	RecoilShotIndex = static_cast<float>(EffectiveShot) + 1.f;
	RecoilShotDecayRate = 0.f;
	LastRecoilFireTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	HumanCharacter->CombatComp->CurShotCount = GetCurrentShotCount();

	if (Weapon->FragWeapon->RecoilCurve == nullptr) return;

	// 上一发还没应用完的后坐力并入本发继续应用
	const float RemainingVert = bIsApplyingRecoil ? (RecoilVertCurFire - RecoilVertLastTick) : 0.f;
	const float RemainingHor  = bIsApplyingRecoil ? (RecoilHorCurFire - RecoilHorLastTick) : 0.f;

	const int32 RandRecoilCurveCount = FMath::RandRange(1, 10); // 曲线横轴只做了 1-10
	const FVector RecoilKick = Weapon->FragWeapon->RecoilCurve->GetVectorValue(EffectiveShot);
	float RecoilRandVert = 0.f;
	float RecoilRandHor = 0.f;
	if (Weapon->FragWeapon->RecoilCurveRandVert) RecoilRandVert = Weapon->FragWeapon->RecoilCurveRandVert->GetFloatValue(RandRecoilCurveCount);
	if (Weapon->FragWeapon->RecoilCurveRandHor) RecoilRandHor = Weapon->FragWeapon->RecoilCurveRandHor->GetFloatValue(RandRecoilCurveCount);

	const float NewKickVert = RecoilKick.Y + RecoilRandVert;
	const float NewKickHor  = RecoilKick.X + RecoilRandHor;

	// 本次开火需要应用的目标后坐力
	RecoilVertCurFire = RemainingVert + NewKickVert;
	RecoilHorCurFire  = RemainingHor + NewKickHor;

	// 累计总后坐力
	RecoilVertTotal += NewKickVert;
	RecoilHorTotal  += NewKickHor;

	bIsApplyingRecoil = true;
	bIsWaitingRecover = false;
	bIsRecoveringRecoil = false;
	RecoilIncCostTime = 0.f;
	RecoilVertLastTick = 0.f;
	RecoilHorLastTick = 0.f;

	RecoilDecTime = 0.f;
	RecoilDecCostTime = 0.f;
}

// 在 RecoilIncTime 时间内，分多帧应用开火后产生的后坐力
void URecoilComponent::PollApplyRecoil(float DeltaSeconds)
{
	if (!bIsApplyingRecoil) return;

	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr) return;
	AWeapon* Weapon = HumanCharacter->CombatComp->GetCurWeapon();
	if (Weapon == nullptr || Weapon->FragWeapon == nullptr)
	{
		ResetRecoil();
		return;
	}

	const float IncTime = Weapon->FragWeapon->RecoilIncTime;
	RecoilIncCostTime = FMath::Min(RecoilIncCostTime + DeltaSeconds, IncTime);

	// 当前帧需要达到的后坐力
	const float Alpha = RecoilIncCostTime / IncTime;
	const float RecoilVertCurTick = FMath::InterpEaseOut(0.f, RecoilVertCurFire, Alpha, 2.f);
	const float RecoilHorCurTick  = FMath::InterpEaseOut(0.f, RecoilHorCurFire, Alpha, 2.f);

	const float StepVert = RecoilVertCurTick - RecoilVertLastTick;
	const float StepHor  = RecoilHorCurTick - RecoilHorLastTick;

	// 推高镜头，并记录镜头后续需要恢复的后坐力
	HumanCharacter->AddControllerPitchInput(-StepVert);
	HumanCharacter->AddControllerYawInput(StepHor);
	CameraRecoilVertTotal += StepVert;
	CameraRecoilHorTotal  += StepHor;

	RecoilVertLastTick = RecoilVertCurTick;
	RecoilHorLastTick  = RecoilHorCurTick;

	if (RecoilIncCostTime >= IncTime)
	{
		bIsApplyingRecoil = false;
		bIsWaitingRecover = true;
	}
}

void URecoilComponent::TryStartRecoverRecoil()
{
	if (!bIsWaitingRecover) return;

	AWeapon* Weapon = (HumanCharacter && HumanCharacter->CombatComp) ? HumanCharacter->CombatComp->GetCurWeapon() : nullptr;
	if (!Weapon || !Weapon->FragWeapon || !GetWorld())
	{
		ResetRecoil();
		return;
	}

	// 停火超过一发间隔后才开始恢复，避免全自动连射在两发子弹之间来回拉扯镜头
	const double FireGrace = static_cast<double>(Weapon->GetFireDelay()) + 0.02;
	if (GetWorld()->GetTimeSeconds() - LastRecoilFireTime < FireGrace)
	{
		return;
	}

	bIsWaitingRecover = false;

	RecoilDecTime = FMath::Clamp(
		FMath::Abs(RecoilVertTotal) / Weapon->FragWeapon->RecoilVertRef_DecTime,
		Weapon->FragWeapon->RecoilMinDecTime,
		Weapon->FragWeapon->RecoilMaxDecTime
	);

	const bool bHasBulletRecoil = !FMath::IsNearlyZero(RecoilVertTotal) || !FMath::IsNearlyZero(RecoilHorTotal);
	const bool bHasCameraRecoil = !FMath::IsNearlyZero(CameraRecoilVertTotal) || !FMath::IsNearlyZero(CameraRecoilHorTotal);
	if (RecoilDecTime > KINDA_SMALL_NUMBER && (bHasBulletRecoil || bHasCameraRecoil))
	{
		bIsRecoveringRecoil = true;
		RecoilDecCostTime = 0.f;

		RecoilVertRecoverTotal = RecoilVertTotal;
		RecoilHorRecoverTotal  = RecoilHorTotal;
		RecoilVertRecoverLastTick = 0.f;
		RecoilHorRecoverLastTick  = 0.f;

		CameraRecoilVertRecoverTotal = CameraRecoilVertTotal;
		CameraRecoilHorRecoverTotal  = CameraRecoilHorTotal;
		CameraRecoilVertRecoverLastTick = 0.f;
		CameraRecoilHorRecoverLastTick  = 0.f;
	}
	else
	{
		bIsRecoveringRecoil = false;
		RecoilVertTotal = 0.f;
		RecoilHorTotal = 0.f;
		CameraRecoilVertTotal = 0.f;
		CameraRecoilHorTotal = 0.f;
	}
}

// 停火后，分多帧恢复后坐力
void URecoilComponent::PollRecoverRecoil(float DeltaSeconds)
{
	if (!bIsRecoveringRecoil) return;

	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr) return;
	AWeapon* Weapon = HumanCharacter->CombatComp->GetCurWeapon();
	if (Weapon == nullptr || Weapon->FragWeapon == nullptr)
	{
		ResetRecoil();
		return;
	}

	RecoilDecCostTime = FMath::Min(RecoilDecCostTime + DeltaSeconds, RecoilDecTime);
	const float Alpha = RecoilDecCostTime / RecoilDecTime;

	// 1. 恢复总后坐力（子弹偏移与准星扩散）
	const float RecoilVertRecoverCurTick = FMath::InterpEaseInOut(0.f, RecoilVertRecoverTotal, Alpha, 2.f);
	const float RecoilHorRecoverCurTick  = FMath::InterpEaseInOut(0.f, RecoilHorRecoverTotal, Alpha, 2.f);
	const float DiffBulletVert = RecoilVertRecoverCurTick - RecoilVertRecoverLastTick;
	const float DiffBulletHor  = RecoilHorRecoverCurTick - RecoilHorRecoverLastTick;

	if (RecoilVertTotal > 0.f) RecoilVertTotal = FMath::Max(0.f, RecoilVertTotal - DiffBulletVert);
	else if (RecoilVertTotal < 0.f) RecoilVertTotal = FMath::Min(0.f, RecoilVertTotal - DiffBulletVert);

	if (RecoilHorTotal > 0.f) RecoilHorTotal = FMath::Max(0.f, RecoilHorTotal - DiffBulletHor);
	else if (RecoilHorTotal < 0.f) RecoilHorTotal = FMath::Min(0.f, RecoilHorTotal - DiffBulletHor);

	RecoilVertRecoverLastTick = RecoilVertRecoverCurTick;
	RecoilHorRecoverLastTick  = RecoilHorRecoverCurTick;

	// 2. 恢复镜头后坐力（只恢复没被玩家手动压掉的部分）
	const float CameraVertRecoverCurTick = FMath::InterpEaseInOut(0.f, CameraRecoilVertRecoverTotal, Alpha, 2.f);
	const float CameraHorRecoverCurTick  = FMath::InterpEaseInOut(0.f, CameraRecoilHorRecoverTotal, Alpha, 2.f);
	const float RawStepCameraVert = CameraVertRecoverCurTick - CameraRecoilVertRecoverLastTick;
	const float RawStepCameraHor  = CameraHorRecoverCurTick - CameraRecoilHorRecoverLastTick;

	float ActualStepVert = 0.f;
	if (CameraRecoilVertTotal > 0.f && RawStepCameraVert > 0.f)
	{
		ActualStepVert = FMath::Min(RawStepCameraVert, CameraRecoilVertTotal);
	}
	else if (CameraRecoilVertTotal < 0.f && RawStepCameraVert < 0.f)
	{
		ActualStepVert = FMath::Max(RawStepCameraVert, CameraRecoilVertTotal);
	}
	if (!FMath::IsNearlyZero(ActualStepVert))
	{
		HumanCharacter->AddControllerPitchInput(ActualStepVert);
		CameraRecoilVertTotal -= ActualStepVert;
	}

	float ActualStepHor = 0.f;
	if (CameraRecoilHorTotal > 0.f && RawStepCameraHor > 0.f)
	{
		ActualStepHor = FMath::Min(RawStepCameraHor, CameraRecoilHorTotal);
	}
	else if (CameraRecoilHorTotal < 0.f && RawStepCameraHor < 0.f)
	{
		ActualStepHor = FMath::Max(RawStepCameraHor, CameraRecoilHorTotal);
	}
	if (!FMath::IsNearlyZero(ActualStepHor))
	{
		HumanCharacter->AddControllerYawInput(-ActualStepHor);
		CameraRecoilHorTotal -= ActualStepHor;
	}

	CameraRecoilVertRecoverLastTick = CameraVertRecoverCurTick;
	CameraRecoilHorRecoverLastTick  = CameraHorRecoverCurTick;

	if (RecoilDecCostTime >= RecoilDecTime)
	{
		bIsRecoveringRecoil = false;
		RecoilVertTotal = 0.f;
		RecoilHorTotal = 0.f;
		CameraRecoilVertTotal = 0.f;
		CameraRecoilHorTotal = 0.f;
	}
}

void URecoilComponent::PollDecayShotCount(float DeltaSeconds)
{
	if (RecoilShotIndex <= 1.f) return;

	AWeapon* Weapon = (HumanCharacter && HumanCharacter->CombatComp) ? HumanCharacter->CombatComp->GetCurWeapon() : nullptr;
	if (!Weapon || !Weapon->FragWeapon || !GetWorld())
	{
		RecoilShotIndex = 1.f;
		RecoilShotDecayRate = 0.f;
		return;
	}

	// 连射或快速点射期间不衰减，停火超过一发间隔后平滑回落至第 1 发
	const double FireGrace = static_cast<double>(Weapon->GetFireDelay()) + 0.03;
	if (GetWorld()->GetTimeSeconds() - LastRecoilFireTime <= FireGrace)
	{
		return;
	}

	if (RecoilShotDecayRate <= 0.f)
	{
		const float RefDecTime = RecoilDecTime > KINDA_SMALL_NUMBER ? RecoilDecTime : Weapon->FragWeapon->RecoilMinDecTime;
		RecoilShotDecayRate = (RecoilShotIndex - 1.f) / RefDecTime;
	}

	RecoilShotIndex = FMath::Max(1.f, RecoilShotIndex - RecoilShotDecayRate * DeltaSeconds);
	if (RecoilShotIndex <= 1.f)
	{
		RecoilShotDecayRate = 0.f;
	}
	HumanCharacter->CombatComp->CurShotCount = GetCurrentShotCount();
}

// 获取子弹偏移
float URecoilComponent::GetCurRecoilVert()
{
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr)
	{
		return RecoilVertTotal;
	}
	
	return RecoilVertTotal * (1 - HumanCharacter->CombatComp->AimingProgress);
}

float URecoilComponent::GetCurRecoilHor()
{
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr)
	{
		return RecoilHorTotal;
	}
	
	return RecoilHorTotal * (1 - HumanCharacter->CombatComp->AimingProgress);
}

// TODO 跳跃、移动速度影响精度
FVector2D URecoilComponent::GetCurSpreadVector()
{
	if (HumanCharacter && HumanCharacter->CombatComp)
	{
		AWeapon* Weapon = HumanCharacter->CombatComp->GetCurWeapon();
		if (Weapon && Weapon->FragWeapon && Weapon->FragWeapon->RecoilCurve)
		{
			FVector RecoilKick = Weapon->FragWeapon->RecoilCurve->GetVectorValue(GetCurrentShotCount());
			
			return FMath::RandPointInCircle(RecoilKick.Z);
		}
	}

	return FVector2D::ZeroVector;
}
