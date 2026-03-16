#include "AutoHostComponent.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/System/UISubsystem.h"
#include "Components/StateTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

UAutoHostComponent::UAutoHostComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsAutoHosting = false;
	LastActiveTime = 0.f;
}

void UAutoHostComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseChar = Cast<ABaseCharacter>(GetOwner());
	if (BaseChar)
	{
		StateTreeComp = BaseChar->FindComponentByClass<UStateTreeComponent>();
		AIPerceptionComp = BaseChar->FindComponentByClass<UAIPerceptionComponent>();
	}
	
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::OnTargetPerceptionUpdated);
	}
}

void UAutoHostComponent::StartAFKCheck()
{
	if (!GetWorld()) return;

	LastActiveTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(AFKCheckTimerHandle, this, &ThisClass::CheckIdleStatus, 1.0f, true);
}

void UAutoHostComponent::UpdateActiveTime()
{
	if (!GetWorld()) return;

	LastActiveTime = GetWorld()->GetTimeSeconds();
 
	if (bIsAutoHosting)
	{
		StopAutoHost();
	}
}

void UAutoHostComponent::CheckIdleStatus()
{
	if (!GetWorld()) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	
	float AFKHostingTime = 30.f;
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		AFKHostingTime = GetDefault<UDevSetting>()->AFKHostingTime;
	}
	
	if (CurrentTime - LastActiveTime > AFKHostingTime)
	{
		if (!bIsAutoHosting)
		{
			StartAutoHost();
		}
	}
}

void UAutoHostComponent::StartAutoHost()
{
	bIsAutoHosting = true;
	
	if (StateTreeComp)
	{
		StateTreeComp->StartLogic();
	}
	
	BroadcastAFKUIState(true);
}

void UAutoHostComponent::StopAutoHost()
{
	bIsAutoHosting = false;
	
	if (StateTreeComp)
	{
		StateTreeComp->StopLogic(TEXT("PlayerInput"));
	}
	
	BroadcastAFKUIState(false);
}

void UAutoHostComponent::BroadcastAFKUIState(bool bIsHosting)
{
	if (!BaseChar) return;
	
	if (ABaseController* BaseController = Cast<ABaseController>(BaseChar->GetController()))
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->OnAFKHosting.Broadcast(bIsHosting);
		}
	}
}

void UAutoHostComponent::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// if (Stimulus.WasSuccessfullySensed())
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("I see you: %s"), *Actor->GetName());
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("Lost sight of: %s"), *Actor->GetName());
	// }
}

AActor* UAutoHostComponent::GetBestPerceivedTarget()
{
	if (AIPerceptionComp == nullptr || BaseChar == nullptr) return nullptr;

	TArray<AActor*> PerceivedActors;
	AIPerceptionComp->GetKnownPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	AActor* BestTarget = nullptr;
	float MinDistSq = FLT_MAX;
	FVector MyLoc = BaseChar->GetActorLocation();

	ABasePlayerState* MyPlayerState = BaseChar->GetPlayerState<ABasePlayerState>();

	for (AActor* Target : PerceivedActors)
	{
		if (Target == nullptr || Target == BaseChar) continue;
		
		ABaseCharacter* TargetBaseCharacter = Cast<ABaseCharacter>(Target);
		if (MyPlayerState && TargetBaseCharacter)
		{
			if (TargetBaseCharacter->bIsDead) continue;
			
			if (ABasePlayerState* TargetPlayerState = TargetBaseCharacter->GetPlayerState<ABasePlayerState>())
			{
				if (MyPlayerState->Team == TargetPlayerState->Team)
				{
					continue;
				}
			}
		}
		
		float DistSq = FVector::DistSquared(MyLoc, Target->GetActorLocation());
		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			BestTarget = Target;
		}
	}
	
	return BestTarget;
}
