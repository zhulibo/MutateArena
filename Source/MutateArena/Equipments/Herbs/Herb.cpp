#include "Herb.h"

#include "MutateArena/GameStates/MutationGameState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/UISubsystem.h"
#include "Net/UnrealNetwork.h"

AHerb::AHerb()
{
	bReplicates = true;

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	SetRootComponent(OverlapSphere);
	OverlapSphere->SetSphereRadius(100.f);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	OverlapSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnSphereOverlap);

	HerbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HerbMesh"));
	HerbMesh->SetupAttachment(RootComponent);
	HerbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HerbMesh->SetCollisionObjectType(ECC::MeshHerb);
	
	Tags.Add(Tags::Herb);
}

void AHerb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Level);
}

void AHerb::BeginPlay()
{
	Super::BeginPlay();
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		UISubsystem->OnRoundStarted.AddUObject(this, &ThisClass::OnRoundStarted);
	}
}

void AHerb::SetLevel(int32 TempLevel)
{
	Level = TempLevel;

	OnRep_Level();
}

void AHerb::OnRep_Level()
{
	if (Level == 2)
	{
		HerbMesh->SetStaticMesh(HerbMeshLevel2);
		
		// 可被人类近战武器销毁
		HerbMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// 可被突变体食用
		OverlapSphere->SetCollisionResponseToChannel(ECC::MeshTeam2, ECollisionResponse::ECR_Overlap);
	}
}

void AHerb::OnRoundStarted()
{
	Destroy();
}

void AHerb::ServerDestroy_Implementation()
{
	Destroy();
}
