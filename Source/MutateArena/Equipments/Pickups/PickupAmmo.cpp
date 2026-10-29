#include "PickupAmmo.h"

#include "Components/SphereComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

void APickupAmmo::BeginPlay()
{
	Super::BeginPlay();

	OverlapSphere->SetCollisionResponseToChannel(ECC::MeshTeam1, ECollisionResponse::ECR_Overlap);
}

void APickupAmmo::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(OtherActor);
	if (HumanCharacter && HumanCharacter->CombatComp)
	{
		if (AWeapon* Slot1Equipment = HumanCharacter->CombatComp->Slot1Equipment)
		{
			Slot1Equipment->SetAmmo(Slot1Equipment->FragWeapon->MagCapacity);
			Slot1Equipment->SetCarriedAmmo(Slot1Equipment->FragWeapon->MaxCarriedAmmo);
			
			Destroy();
		}
	}
}
