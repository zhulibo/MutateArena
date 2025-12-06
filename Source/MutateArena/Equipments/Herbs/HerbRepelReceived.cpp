#include "HerbRepelReceived.h"

#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/MutantCharacter.h"

AHerbRepelReceived::AHerbRepelReceived()
{
}

void AHerbRepelReceived::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			SetLevel(2);
		});
		// 生成X秒内处于生长状态，只能被双方看到，X秒后可被双方交互
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 15.f, false);
	}
}

void AHerbRepelReceived::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(OtherActor))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = MutantCharacter->GetASC())
		{
			FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
			EffectContext.AddSourceObject(this);
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(HerbRepelReceivedEffect, 1.f, EffectContext);
			if (SpecHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
			}
		}

		Destroy();
	}
}
