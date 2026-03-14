#include "GC_Weapon_Fire.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Characters/AnimInstHuman.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AudioModulationStatics.h"
#include "MetaSoundSource.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/System/AssetSubsystem.h"

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

    // 播放角色动画
    if (UAnimInstHuman* HumanAnimInst = Cast<UAnimInstHuman>(HumanChar->GetMesh()->GetAnimInstance()))
    {
        if (HumanAnimInst->Montage_IsPlaying(CurWeapon->ADSMontage_C))
        {
        }
        else
        {
            UAnimMontage* CharMontage = CombatComp->IsAiming() ? CurWeapon->FireADSMontage_C : CurWeapon->FireMontage_C;
            if (!HumanChar->IsLocallyControlled())
            {
                CharMontage = CurWeapon->FireMontage_C; // TODO 非本地瞄准动画暂时禁用了
            }
            HumanAnimInst->Montage_Play(CharMontage); 
        }
    }

	// 播放武器动画
    if (UAnimInstEquipment* EquipmentAnimInst = CurWeapon->GetEquipmentAnimInst())
    {
        UAnimMontage* WeapMontage = CombatComp->IsAiming() ? CurWeapon->FireADSMontage_E : CurWeapon->FireMontage_E;
        if (!HumanChar->IsLocallyControlled())
        {
            WeapMontage = CurWeapon->FireMontage_E; // TODO 非本地瞄准动画暂时禁用了
        }
        EquipmentAnimInst->Montage_Play(WeapMontage);
    }

    if (USceneComponent* WeaponMesh = CurWeapon->EquipmentMesh)
    {
        if (CurWeapon->MuzzleFlashEffect)
        {
            UNiagaraFunctionLibrary::SpawnSystemAttached(
                CurWeapon->MuzzleFlashEffect,
                WeaponMesh,
                SOCKET_MUZZLE,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::SnapToTarget,
                true
            );
        }

        UGameplayStatics::SpawnSoundAttached(CurWeapon->FireSound, WeaponMesh, SOCKET_MUZZLE);
        UGameplayStatics::SpawnSoundAttached(CurWeapon->MechSound, WeaponMesh, SOCKET_MUZZLE);
    }

    if (HumanChar->IsLocallyControlled())
    {
        float Volume = 0.6f;
        float LastTime = CurWeapon->bIsAutomatic ? 1.2f : 0.5f;

        // 判断是否到达低弹量阈值
        if (CurWeapon->Ammo <= (CurWeapon->FireRate / 60.f) * LastTime)
        {
            Volume = 2.f;
        }

        UAssetSubsystem* AssetSubsystem = HumanChar->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
        if (AssetSubsystem && AssetSubsystem->CommonAsset && AssetSubsystem->CommonAsset->CB_EquipmentMech)
        {
            UAudioModulationStatics::SetGlobalBusMixValue(
                MyTarget,
                AssetSubsystem->CommonAsset->CB_EquipmentMech,
                Volume,
                0.f
            );
        }
    }

    return Super::OnExecute_Implementation(MyTarget, Parameters);
}
