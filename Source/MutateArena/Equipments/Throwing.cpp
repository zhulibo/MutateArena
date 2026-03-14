#include "Throwing.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"

AThrowing::AThrowing()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere->SetSphereRadius(10.f); // 离地面的距离

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->SetIsReplicated(true);
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->SetAutoActivate(false);
}

void AThrowing::BeginPlay()
{
	Super::BeginPlay();
}

void AThrowing::ThrowOut()
{
	EquipmentState = EEquipmentState::Thrown;

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	CollisionSphere->DetachFromComponent(DetachRules);

	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		if (HumanCharacter->HasAuthority() || HumanCharacter->IsLocallyControlled())
		{
			FVector ThrowVector = HumanCharacter->GetViewRotation().Vector();
			ThrowVector.Z += 0.1f;
			ProjectileMovement->Velocity = ThrowVector * 1500.f;
			ProjectileMovement->Activate();
		}
	}
	
	SetReplicateMovement(true);
}

void AThrowing::OnRep_EquipmentState(EEquipmentState OldState)
{
	Super::OnRep_EquipmentState(OldState);
	
	if (EquipmentState == EEquipmentState::Thrown)
	{
		if (GetLocalRole() == ROLE_SimulatedProxy)
		{
			ThrowOut();
		}
	}
}
