#include "Grenade.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "NiagaraFunctionLibrary.h"

AGrenade::AGrenade()
{
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.6f;
}

void AGrenade::ThrowOut()
{
	Super::ThrowOut();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::ServerExplode, 3.f);
	}
}

void AGrenade::ServerExplode()
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	ABaseController* BaseController = HumanCharacter ? Cast<ABaseController>(HumanCharacter->GetController()) : nullptr;

	if (HumanCharacter && BaseController)
	{
		TArray<AActor*> IgnoreActors;
		if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();
		if (ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>())
		{
			for (int32 i = 0; i < BaseGameState->GetPlayerStates(OwnerTeam).Num(); ++i)
			{
				if (BaseGameState->GetPlayerStates(OwnerTeam)[i])
				{
					IgnoreActors.AddUnique(BaseGameState->GetPlayerStates(OwnerTeam)[i]->GetPawn());
				}
			}
		}

		UGameplayStatics::ApplyRadialDamageWithFalloff(
			this,
			Damage,
			100.f,
			GetActorLocation(),
			DamageInnerRadius,
			DamageOuterRadius,
			1.f,
			UDamageType::StaticClass(),
			IgnoreActors,
			this,
			BaseController
		);
	}

	MulticastExplodeEffects();

	SetLifeSpan(2.0f);
}

void AGrenade::MulticastExplodeEffects_Implementation()
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}
	
	if (ExplodeEffect)
	{
		ExplodeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplodeEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}
}
