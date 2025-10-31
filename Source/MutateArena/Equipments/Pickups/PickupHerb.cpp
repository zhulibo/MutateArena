#include "PickupHerb.h"

#include "Components/SphereComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/PlayerStates/MutationPlayerState.h"
#include "Net/UnrealNetwork.h"

APickupHerb::APickupHerb()
{
	PickupMesh->SetCollisionObjectType(ECC_MESH_PICKUP_HREB);
	
	Tags.Add(TAG_PICKUP_HERB);
}

void APickupHerb::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Level);
}

void APickupHerb::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			SetLevel(2);
		});
		float X = FMath::RandRange(15.f, 20.f);
		// 生成X秒内处于生长状态，只能被双方看到，X秒后可被双方交互
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, X, false);
	}
}

void APickupHerb::SetLevel(int32 TempLevel)
{
	Level = TempLevel;

	OnRep_Level();
}

void APickupHerb::OnRep_Level()
{
	if (Level == 2)
	{
		PickupMesh->SetStaticMesh(PickupMeshLevel2);
		
		// 可被人类近战武器销毁
		PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// 可被突变体食用
		OverlapSphere->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Overlap);
	}
}

void APickupHerb::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(OtherActor))
	{
		if (AMutationPlayerState* MutationPlayerState = MutantCharacter->GetPlayerState<AMutationPlayerState>())
		{
			MutationPlayerState->SetRage(MutationPlayerState->Rage + MutationPlayerState->RageLevel3);
		}

		Destroy();
	}
}

void APickupHerb::ServerDestroy_Implementation()
{
	Destroy();
}
