#include "GC_Weapon_Fire.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Characters/AnimInstHumanBase.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

UGC_Weapon_Fire::UGC_Weapon_Fire()
{
}

bool UGC_Weapon_Fire::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
    AHumanCharacter* HumanChar = Cast<AHumanCharacter>(MyTarget);
    AWeapon* CurWeapon = Cast<AWeapon>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	UCombatComponent* CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
    
    if (!HumanChar || !CurWeapon || !CombatComp)
    {
        return Super::OnExecute_Implementation(MyTarget, Parameters);
    }

    // 根据当前视角选择 1P (Mesh1P) 或 3P (BodyMesh) 动画实例，触发程序化开火后坐力脉冲
    if (USkeletalMeshComponent* TargetCharMesh = HumanChar->bIsCurrentViewTarget ? HumanChar->Mesh1P : HumanChar->BodyMesh.Get())
    {
        if (UAnimInstHumanBase* HumanAnimInst = Cast<UAnimInstHumanBase>(TargetCharMesh->GetAnimInstance()))
        {
            HumanAnimInst->AddRecoilImpulse(CurWeapon->FragWeapon);
        }
    }

    // 同步触发 1P 与 3P 武器网格体的程序化机械构件往复运动
    if (UAnimInstEquipment* EquipAnimInst1P = CurWeapon->GetEquipmentAnimInst1P())
    {
        EquipAnimInst1P->TriggerFireMotion();
    }
    if (UAnimInstEquipment* EquipAnimInst = CurWeapon->GetEquipmentAnimInst())
    {
        EquipAnimInst->TriggerFireMotion();
    }

	if (USceneComponent* WeaponMesh = CurWeapon->GetVisibleEquipmentMesh())
    {
        if (CurWeapon->FragWeapon->MuzzleFlashEffect)
        {
            UNiagaraFunctionLibrary::SpawnSystemAttached(
                CurWeapon->FragWeapon->MuzzleFlashEffect,
                WeaponMesh,
                Sockets::Muzzle,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::SnapToTarget,
                true
            );
        }

        UGameplayStatics::SpawnSoundAttached(CurWeapon->FragWeapon->FireSound, WeaponMesh, Sockets::Muzzle);
    }

	CurWeapon->TriggerShellEject();
	CurWeapon->TriggerBoltAction();

	if (!HumanChar->IsLocallyControlled() && !HumanChar->HasAuthority())
	{
		const int32 ShotCount = FMath::Max(1, FMath::RoundToInt(Parameters.RawMagnitude));
		// AbilityLevel 传递本次开火种子；RawMagnitude 仍用于查询后坐力曲线。
		const int32 SpreadSeed = Parameters.AbilityLevel > 0
			? Parameters.AbilityLevel
			: ShotCount;
		CurWeapon->SpawnProjectile(Parameters.Location, ShotCount, SpreadSeed);
	}

    return Super::OnExecute_Implementation(MyTarget, Parameters);
}
