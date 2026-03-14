#include "Pickup.h"

#include "MutateArena/GameStates/MutationGameState.h"
#include "Components/SphereComponent.h"
#include "MutateArena/System/UISubsystem.h"

APickup::APickup()
{
	bReplicates = true;

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	SetRootComponent(OverlapSphere);
	OverlapSphere->SetSphereRadius(100.f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	// 使补给箱（自身为WorldDynamic）互相阻挡，补给箱生成策略 DontSpawnIfColliding
	OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	OverlapSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnSphereOverlap);
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(RootComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APickup::BeginPlay()
{
	Super::BeginPlay();
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		UISubsystem->OnRoundStarted.AddUObject(this, &ThisClass::OnRoundStarted);
	}
}

void APickup::OnRoundStarted()
{
	Destroy();
}
