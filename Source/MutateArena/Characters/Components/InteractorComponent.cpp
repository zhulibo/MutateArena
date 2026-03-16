#include "InteractorComponent.h"
#include "Camera/CameraComponent.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/Interfaces/Interactable.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInteractorComponent::TraceInteractTarget(FHitResult& OutHit)
{
	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (!OwnerCharacter) return;
	
	if (!GetWorld()) return;

	UCameraComponent* Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera) return;

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * 160.f; // 设置检测距离为 160.f

	FCollisionQueryParams QueryParams;
	TArray<AActor*> TeamPlayers;

	if (ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>())
	{
		QueryParams.AddIgnoredActors(BaseGameState->AllEquipments);

		// 忽略队友
		TArray<ABasePlayerState*> PlayerStates = BaseGameState->GetPlayerStates({});
		for (int32 i = 0; i < PlayerStates.Num(); ++i)
		{
			if (PlayerStates[i] && PlayerStates[i]->GetHealth() > 0.f)
			{
				TeamPlayers.AddUnique(PlayerStates[i]->GetPawn());
			}
		}
	}
	QueryParams.AddIgnoredActors(TeamPlayers);

	GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		ECollisionChannel::ECC_Visibility,
		FCollisionShape::MakeSphere(10.f),
		QueryParams
	);
}

void UInteractorComponent::InteractStarted(const FInputActionValue& Value)
{
	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	FHitResult OutHit;
	TraceInteractTarget(OutHit);

	if (OutHit.bBlockingHit)
	{
		if (IInteractable* Target = Cast<IInteractable>(OutHit.GetActor()))
		{
			if (Target->CanInteract(OwnerCharacter))
			{
				InteractTarget = OutHit.GetActor();

				ABaseController* BaseController = Cast<ABaseController>(OwnerCharacter->GetController());
				if (BaseController && BaseController->IsLocalController())
				{
					if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(
						BaseController->GetLocalPlayer()))
					{
						UISubsystem->OnInteractStarted.Broadcast();
					}
				}
				return;
			}
		}
	}
	InteractTarget = nullptr;
}

void UInteractorComponent::InteractOngoing(const FInputActionValue& Value)
{
	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	if (InteractTarget != nullptr)
	{
		FHitResult OutHit;
		TraceInteractTarget(OutHit);
		if (OutHit.bBlockingHit && InteractTarget == OutHit.GetActor())
		{
			if (IInteractable* Target = Cast<IInteractable>(OutHit.GetActor()))
			{
				if (Target->CanInteract(OwnerCharacter))
				{
					return; // 目标有效，继续交互
				}
			}
		}
	}

	InteractTarget = nullptr;
	if (ABaseController* BaseController = Cast<ABaseController>(OwnerCharacter->GetController()))
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->OnInteractEnded.Broadcast();
		}
	}
}

void UInteractorComponent::InteractTriggered(const FInputActionValue& Value)
{
	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	ABaseController* BaseController = Cast<ABaseController>(OwnerCharacter->GetController());
	if (BaseController && InteractTarget)
	{
		ServerInteractTriggered(InteractTarget);
	}
}

void UInteractorComponent::ServerInteractTriggered_Implementation(AActor* TempInteractTarget)
{
	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (!OwnerCharacter || !TempInteractTarget) return;

	if (IInteractable* Target = Cast<IInteractable>(TempInteractTarget))
	{
		Target->OnInteract_Server(OwnerCharacter);
	}
}

void UInteractorComponent::InteractCompleted(const FInputActionValue& Value)
{
	InteractTarget = nullptr;
	if (ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner()))
	{
		if (ABaseController* BaseController = Cast<ABaseController>(OwnerCharacter->GetController()))
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
			{
				UISubsystem->OnInteractEnded.Broadcast();
			}
		}
	}
}

void UInteractorComponent::InteractCanceled(const FInputActionValue& Value)
{
	InteractCompleted(Value);
}
