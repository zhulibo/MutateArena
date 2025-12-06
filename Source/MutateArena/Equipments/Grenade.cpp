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

	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter)
	{
		if (UCameraComponent* CameraComponent = HumanCharacter->FindComponentByClass<UCameraComponent>())
		{
			FVector ThrowVector = CameraComponent->GetForwardVector();
			ThrowVector.Z += 0.1;
			ProjectileMovement->Velocity = ThrowVector * 1500.f;
			ProjectileMovement->Activate();
		}
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::Explode, 3.f);
}

void AGrenade::Explode()
{
	// 不能直接销毁，同步到模拟端无法执行
	SetLifeSpan(1.f);
	
	if (BaseController == nullptr)
	{
		if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
		if (HumanCharacter) BaseController = Cast<ABaseController>(HumanCharacter->GetController());
	}
	if (BaseController && HasAuthority())
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
			this, // World context object
			Damage, // BaseDamage
			100.f, // MinimumDamage
			GetActorLocation(), // Origin
			DamageInnerRadius, // DamageInnerRadius
			DamageOuterRadius, // DamageOuterRadius
			1.f, // DamageFalloff
			UDamageType::StaticClass(), // DamageTypeClass
			IgnoreActors, // IgnoreActors
			this, // DamageCauser
			BaseController // InstigatorController
		);
	}

	ExplodeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplodeEffect,
		GetActorLocation(),
		GetActorRotation()
	);

	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}

	Destroy();
}
