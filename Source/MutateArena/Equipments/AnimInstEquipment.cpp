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

	bIsPlayingWeaponMontage = IsAnyMontagePlaying();
	if (Equipment == nullptr) return;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(Equipment->GetOwner());
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr || HumanCharacter->CombatComp->GetCurWeapon() == nullptr) return;

	bIsAiming = HumanCharacter->CombatComp->bIsAiming;
}

void UAnimInstEquipment::TriggerFireMotion()
{
	if (OwningWeapon == nullptr) OwningWeapon = Cast<AWeapon>(GetOwningActor());
	if (!OwningWeapon || !OwningWeapon->FragWeapon) return;

	bSuppressFireMotion = false;
	if (!bIsPlayingWeaponMontage)
	{
		MontageSuppressAlpha = 1.f;
	}

	for (const FWeaponFirePartMotion& Motion : OwningWeapon->FragWeapon->FirePartMotions)
	{
		const int32 BoneIdx = static_cast<int32>(Motion.TargetBone);
		if (BoneIdx >= 0 && BoneIdx < MechBoneCount && Motion.CycleDuration > 0.f)
		{
			ElapsedTime[BoneIdx] = 0.f;
		}
	}
}

void UAnimInstEquipment::StopFireMotion(bool bImmediate)
{
	if (OwningWeapon == nullptr) OwningWeapon = Cast<AWeapon>(GetOwningActor());
	if (bImmediate || !OwningWeapon || !OwningWeapon->FragWeapon)
	{
		bSuppressFireMotion = false;
		for (int32 i = 0; i < MechBoneCount; ++i)
		{
			ElapsedTime[i] = -1.f;
		}
		return;
	}

	bool bHasActiveMotion = false;
	for (const FWeaponFirePartMotion& Motion : OwningWeapon->FragWeapon->FirePartMotions)
	{
		const int32 BoneIdx = static_cast<int32>(Motion.TargetBone);
		if (BoneIdx >= 0 && BoneIdx < MechBoneCount && ElapsedTime[BoneIdx] >= 0.f)
		{
			if (ElapsedTime[BoneIdx] <= Motion.GetSafeStartDelay())
			{
				ElapsedTime[BoneIdx] = -1.f;
			}
			else
			{
				bHasActiveMotion = true;
			}
		}
	}
	bSuppressFireMotion = bHasActiveMotion;
}

void UAnimInstEquipment::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// 换弹蒙太奇播放或外部打断期间平滑衰减驱动权重，避免中途打断往复运动时产生跳变
	const float TargetSuppress = (bIsPlayingWeaponMontage || bSuppressFireMotion) ? 0.f : 1.f;
	MontageSuppressAlpha = FMath::FInterpTo(MontageSuppressAlpha, TargetSuppress, DeltaSeconds, 25.f);
	if (bSuppressFireMotion && MontageSuppressAlpha <= 0.01f)
	{
		bSuppressFireMotion = false;
		for (int32 i = 0; i < MechBoneCount; ++i)
		{
			ElapsedTime[i] = -1.f;
		}
	}

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
		*PartLocations[i] = FVector::ZeroVector;
		*PartRotations[i] = FRotator::ZeroRotator;
		*PartAlphas[i] = 0.f;
	}

	if (!OwningWeapon || !OwningWeapon->FragWeapon) return;

	for (const FWeaponFirePartMotion& Motion : OwningWeapon->FragWeapon->FirePartMotions)
	{
		const int32 BoneIdx = static_cast<int32>(Motion.TargetBone);
		if (BoneIdx < 0 || BoneIdx >= MechBoneCount || ElapsedTime[BoneIdx] < 0.f)
		{
			continue;
		}

		ElapsedTime[BoneIdx] += DeltaSeconds;
		if (ElapsedTime[BoneIdx] <= Motion.GetTotalDuration())
		{
			const float EffectivePhase = Motion.EvaluatePhase(ElapsedTime[BoneIdx]) * MontageSuppressAlpha;
			*PartLocations[BoneIdx] = Motion.MaxLocationOffset * EffectivePhase;
			*PartRotations[BoneIdx] = Motion.MaxRotationOffset * EffectivePhase;
			*PartAlphas[BoneIdx] = (EffectivePhase > KINDA_SMALL_NUMBER) ? 1.f : 0.f;
		}
		else
		{
			ElapsedTime[BoneIdx] = -1.f;
		}
	}
}
