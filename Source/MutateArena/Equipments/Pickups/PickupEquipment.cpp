#include "PickupEquipment.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Equipment.h"

void APickupEquipment::BeginPlay()
{
	Super::BeginPlay();

	OverlapSphere->SetCollisionResponseToChannel(ECC::MeshTeam1, ECollisionResponse::ECR_Overlap);

	if (HasAuthority())
	{
		Equipment = GetWorld()->SpawnActor<AEquipment>(EquipmentClass);

		// 放入补给箱中时，将其设为 Holstered 状态以隐藏模型与阴影
		Equipment->SetState(EEquipmentState::Holstered);
	}
}

void APickupEquipment::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (AHumanCharacter* OverlapHumanCharacter = Cast<AHumanCharacter>(OtherActor))
	{
		OverlapHumanCharacter->GivePickupEquipment(this);
	}
}
