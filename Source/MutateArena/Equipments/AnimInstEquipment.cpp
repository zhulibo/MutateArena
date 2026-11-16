#include "AnimInstEquipment.h"

#include "Equipment.h"
#include "Weapon.h"
#include "MutateArena/Characters/AnimInstHuman.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

UAnimInstEquipment::UAnimInstEquipment()
{
}

void UAnimInstEquipment::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Equipment == nullptr) Equipment = Cast<AEquipment>(GetOwningActor());
	if (OwningWeapon == nullptr) OwningWeapon = Cast<AWeapon>(GetOwningActor());

	// 主线程按 EWeaponMechBone 枚举索引缓存构件运动配置与蒙太奇播放状态，供工作线程无锁安全求解
	bIsPlayingWeaponMontage = IsAnyMontagePlaying();
	FMemory::Memzero(bBoneConfigured, sizeof(bBoneConfigured));
	if (OwningWeapon && OwningWeapon->FragWeapon)
	{
		for (const FWeaponFirePartMotion& Motion : OwningWeapon->FragWeapon->FirePartMotions)
		{
			const int32 BoneIdx = static_cast<int32>(Motion.TargetBone);
			if (BoneIdx >= 0 && BoneIdx < MechBoneCount)
			{
				CachedMotions[BoneIdx] = Motion;
				bBoneConfigured[BoneIdx] = true;
			}
		}
	}

	if (Equipment == nullptr) return;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(Equipment->GetOwner());
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr || HumanCharacter->CombatComp->GetCurWeapon() == nullptr) return;

	bIsAiming = HumanCharacter->CombatComp->bIsAiming;
}

void UAnimInstEquipment::TriggerFireMotion()
{
	bool bAnyCached = false;
	for (int32 i = 0; i < MechBoneCount; ++i)
	{
		if (bBoneConfigured[i])
		{
			KickStartPhase[i] = CurrentPhase[i];
			ElapsedTime[i] = 0.f;
			bAnyCached = true;
		}
	}

	// 首帧尚未执行 NativeUpdateAnimation 时回退读取 OwningWeapon，避免首枪漏触发
	if (!bAnyCached)
	{
		if (const AWeapon* Weapon = OwningWeapon ? OwningWeapon.Get() : Cast<AWeapon>(GetOwningActor()))
		{
			if (Weapon->FragWeapon)
			{
				for (const FWeaponFirePartMotion& Motion : Weapon->FragWeapon->FirePartMotions)
				{
					const int32 BoneIdx = static_cast<int32>(Motion.TargetBone);
					if (BoneIdx >= 0 && BoneIdx < MechBoneCount)
					{
						KickStartPhase[BoneIdx] = 0.f;
						ElapsedTime[BoneIdx] = 0.f;
					}
				}
			}
		}
	}
}

void UAnimInstEquipment::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// 换弹蒙太奇播放期间平滑衰减驱动权重，避免中途打断往复运动时产生跳变
	const float TargetSuppress = bIsPlayingWeaponMontage ? 0.f : 1.f;
	MontageSuppressAlpha = FMath::FInterpTo(MontageSuppressAlpha, TargetSuppress, DeltaSeconds, 25.f);

	// 按 EWeaponMechBone 枚举顺序映射输出通道
	static_assert(MechBoneCount == 7, "Update bone channel pointer tables when EWeaponMechBone changes.");
	FVector* PartLocations[MechBoneCount] = {
		&BarrelLocation, &TriggerLocation, &ChargingHandleLocation,
		&BoltLocation, &PumpLocation, &DustCoverLocation, &SlideLocation
	};
	FRotator* PartRotations[MechBoneCount] = {
		&BarrelRotation, &TriggerRotation, &ChargingHandleRotation,
		&BoltRotation, &PumpRotation, &DustCoverRotation, &SlideRotation
	};
	float* PartAlphas[MechBoneCount] = {
		&BarrelAlpha, &TriggerAlpha, &ChargingHandleAlpha,
		&BoltAlpha, &PumpAlpha, &DustCoverAlpha, &SlideAlpha
	};

	for (int32 i = 0; i < MechBoneCount; ++i)
	{
		if (bBoneConfigured[i] && ElapsedTime[i] >= 0.f)
		{
			const FWeaponFirePartMotion& Motion = CachedMotions[i];
			const float Duration = FMath::Max(0.01f, Motion.CycleDuration);

			ElapsedTime[i] += DeltaSeconds;
			if (ElapsedTime[i] <= Duration)
			{
				const float NormalizedTime = FMath::Clamp(ElapsedTime[i] / Duration, 0.f, 1.f);
				const float BaseKickRatio = FMath::Clamp(Motion.KickTimeRatio, 0.05f, 0.85f);
				// 按剩余行程比例折算去程耗时，确保半途接续顶回最大行程时保持干脆的速度感
				const float RemainingTravel = FMath::Clamp(1.f - KickStartPhase[i], 0.2f, 1.f);
				const float KickRatio = BaseKickRatio * RemainingTravel;
				const float HoldRatio = FMath::Clamp(Motion.HoldTimeRatio, 0.f, 0.6f);
				const float HoldEnd = FMath::Min(KickRatio + HoldRatio, 0.99f);

				float Phase = 0.f;
				if (NormalizedTime < KickRatio)
				{
					// 1. 去程：从 KickStartPhase 无缝急起推至 1.0
					const float Alpha = NormalizedTime / KickRatio;
					Phase = FMath::Lerp(KickStartPhase[i], 1.f, FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.5f));
				}
				else if (NormalizedTime < HoldEnd)
				{
					// 2. 驻留：后死点保持（如 GM6 管退滞留）
					Phase = 1.f;
				}
				else
				{
					// 3. 复进：平滑回弹归位
					const float ReturnDuration = FMath::Max(KINDA_SMALL_NUMBER, 1.f - HoldEnd);
					const float ReturnAlpha = FMath::Clamp((NormalizedTime - HoldEnd) / ReturnDuration, 0.f, 1.f);
					Phase = 1.f - FMath::InterpEaseInOut(0.f, 1.f, ReturnAlpha, 1.8f);
				}

				CurrentPhase[i] = Phase;

				const float EffectivePhase = Phase * MontageSuppressAlpha;
				*PartLocations[i] = Motion.MaxLocationOffset * EffectivePhase;
				*PartRotations[i] = Motion.MaxRotationOffset * EffectivePhase;
				*PartAlphas[i] = (EffectivePhase > KINDA_SMALL_NUMBER) ? 1.f : 0.f;
				continue;
			}
		}

		// 未配置、静止或单周期结束时复位状态与输出
		ElapsedTime[i] = -1.f;
		CurrentPhase[i] = 0.f;
		KickStartPhase[i] = 0.f;
		*PartLocations[i] = FVector::ZeroVector;
		*PartRotations[i] = FRotator::ZeroRotator;
		*PartAlphas[i] = 0.f;
	}
}
