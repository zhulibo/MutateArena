#include "Throwable.h"

#include "MetaSoundSource.h"
#include "Components/SphereComponent.h"
#include "Data/EquipmentAsset.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/System/AssetSubsystem.h"

AThrowable::AThrowable()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere->SetSphereRadius(10.f); // 离地面的距离

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->SetIsReplicated(true);
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->SetAutoActivate(false);
}

void AThrowable::BeginPlay()
{
	Super::BeginPlay();
	
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	check(AssetSubsystem);

	FragThrowable = EquipmentAsset->FindFragmentByClass<UFragThrowable>();
	check(FragThrowable);
	
	if (ProjectileMovement)
	{
		ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AThrowable::OnProjectileBounce);
	}
}

void AThrowable::ThrowOut()
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

void AThrowable::OnRep_EquipmentState(EEquipmentState OldState)
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

void AThrowable::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (!FragThrowable->HitSound || bHasBounced) return;

	bHasBounced = true;

	UGameplayStatics::SpawnSoundAttached(
		FragThrowable->HitSound, 
		GetRootComponent(), 
		NAME_None, 
		FVector::ZeroVector, 
		EAttachLocation::KeepRelativeOffset, 
		true
	);
}
