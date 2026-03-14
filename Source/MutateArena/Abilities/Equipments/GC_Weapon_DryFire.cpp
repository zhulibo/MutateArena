#include "GC_Weapon_DryFire.h"

#include "MetaSoundSource.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Mutants/MutantTank.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/AssetSubsystem.h"

UGC_Weapon_DryFire::UGC_Weapon_DryFire()
{
}

bool UGC_Weapon_DryFire::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(MyTarget);
	const AWeapon* CurWeapon = Cast<AWeapon>(Parameters.SourceObject.Get());
	
	if (!HumanCharacter || !HumanCharacter->GetGameInstance() || !CurWeapon)
	{
		return Super::OnExecute_Implementation(MyTarget, Parameters);
	}

	UAssetSubsystem* AssetSubsystem = HumanCharacter->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (!AssetSubsystem || !AssetSubsystem->EquipmentAsset)
	{
		return Super::OnExecute_Implementation(MyTarget, Parameters);
	}

	UGameplayStatics::PlaySoundAtLocation(CurWeapon, AssetSubsystem->EquipmentAsset->DryFireSound, CurWeapon->GetActorLocation());
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
