#include "PickupAmmo.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Weapon.h"

void APickupAmmo::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(OtherActor);
	if (HumanCharacter && HumanCharacter->CombatComponent)
	{
		if (AWeapon* PrimaryEquipment = HumanCharacter->CombatComponent->PrimaryEquipment)
		{
			PrimaryEquipment->MulticastSetFullAmmo();

			Destroy();
		}
	}
}
