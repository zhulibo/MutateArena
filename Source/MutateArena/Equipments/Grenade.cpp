#include "Grenade.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "Camera/CameraComponent.h"
#include "Data/EquipmentAsset.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "GameFramework/DamageType.h"

AGrenade::AGrenade()
{
}

void AGrenade::BeginPlay()
{
	Super::BeginPlay();

	FragGrenade = EquipmentAsset->FindFragmentByClass<UFragGrenade>();
	check(FragGrenade);
}

void AGrenade::ThrowOut()
{
	Super::ThrowOut();

	if (HasAuthority())
	{
		if (FragThrowable->FuseTime > 0.f)
		{
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::ServerExplode, FragThrowable->FuseTime);
		}
		else
		{
			ServerExplode();
		}
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
			TArray<ABasePlayerState*> PlayerStates;
			BaseGameState->GetPlayerStates(OwnerTeam, PlayerStates);
			for (int32 i = 0; i < PlayerStates.Num(); ++i)
			{
				if (PlayerStates[i])
				{
					IgnoreActors.AddUnique(PlayerStates[i]->GetPawn());
				}
			}
		}

		UGameplayStatics::ApplyRadialDamageWithFalloff(
			this,
			FragGrenade->Damage,
			100.f,
			GetActorLocation(),
			FragGrenade->DamageInnerRadius,
			FragGrenade->DamageOuterRadius,
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

	if (FragThrowable->ExplodeEffect)
	{
		ExplodeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			FragThrowable->ExplodeEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	if (FragThrowable->ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FragThrowable->ExplodeSound, GetActorLocation());
	}
}
