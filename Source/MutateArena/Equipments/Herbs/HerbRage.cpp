#include "HerbRage.h"

#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/PlayerStates/MutationPlayerState.h"

AHerbRage::AHerbRage()
{
}

void AHerbRage::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			SetLevel(2);
		});
		float X = FMath::RandRange(10.f, 20.f);
		// 生成X秒内处于生长状态，只能被双方看到，X秒后可被双方交互
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, X, false);
	}
}

void AHerbRage::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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
