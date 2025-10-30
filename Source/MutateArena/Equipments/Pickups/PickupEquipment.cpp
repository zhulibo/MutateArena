#include "PickupEquipment.h"

#include "Components/SphereComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Equipment.h"

void APickupEquipment::BeginPlay()
{
	Super::BeginPlay();
	
	OverlapSphere->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Overlap);

	if (HasAuthority())
	{
		Equipment = GetWorld()->SpawnActor<AEquipment>(EquipmentClass);
		Equipment->MulticastHiddenMesh();
	}
}

void APickupEquipment::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 取消补给箱装备隐藏
	if (Equipment)
	{
		Equipment->EquipmentMesh->SetVisibility(true);
	}

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(OtherActor);
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		HumanCharacter->bCanSwitchLoadout = false;
		HumanCharacter->ServerGivePickupEquipment(this);
	}
}
