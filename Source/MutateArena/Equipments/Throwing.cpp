#include "Throwing.h"

#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

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

void AThrowing::ThrowOut()
{
	EquipmentState = EEquipmentState::Thrown;

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	CollisionSphere->DetachFromComponent(DetachRules);
}
