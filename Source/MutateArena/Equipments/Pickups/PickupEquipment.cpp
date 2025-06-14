#include "PickupEquipment.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Equipment.h"

void APickupEquipment::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Equipment = GetWorld()->SpawnActor<AEquipment>(EquipmentClass);
		Equipment->MulticastHiddenMesh();
	}
}

void APickupEquipment::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(OtherActor);
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		HumanCharacter->bCanSwitchLoadout = false;
		HumanCharacter->ServerGivePickupEquipment(this);
	}
}
