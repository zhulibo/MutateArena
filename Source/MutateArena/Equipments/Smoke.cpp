#include "Smoke.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Sub/SmokeBlocker.h"

ASmoke::ASmoke()
{
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.6f;
}

void ASmoke::ThrowOut()
{
	Super::ThrowOut();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::ServerExplode, 2.f);
	}
}

void ASmoke::ServerExplode()
{
	MulticastExplodeEffects();
	
	SetLifeSpan(Time);
}

void ASmoke::MulticastExplodeEffects_Implementation()
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	// 播放特效和声音
	SpawnedSmokeEffect= UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplodeEffect,
		GetActorLocation(),
		GetActorRotation()
	);

	if (SpawnedSmokeEffect)
	{
		SpawnedSmokeEffect->SetVariableFloat(TEXT("Time"), Time);
	}

	SpawnedSmokeSound = UGameplayStatics::SpawnSoundAtLocation(
		this, 
		ExplodeSound, 
		GetActorLocation()
	);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedSmokeBlocker = GetWorld()->SpawnActor<ASmokeBlocker>(
		ASmokeBlocker::StaticClass(), 
		GetActorLocation(), 
		GetActorRotation(), 
		SpawnParams
	);
	if (SpawnedSmokeBlocker)
	{
		SpawnedSmokeBlocker->SetLifeSpan(Time);
	}
}

void ASmoke::Destroyed()
{
	if (SpawnedSmokeEffect)
	{
		SpawnedSmokeEffect->DestroyComponent();
		SpawnedSmokeEffect = nullptr;
	}

	if (SpawnedSmokeSound)
	{
		SpawnedSmokeSound->Stop();
		SpawnedSmokeSound->DestroyComponent();
		SpawnedSmokeSound = nullptr;
	}
	
	if (SpawnedSmokeBlocker)
	{
		SpawnedSmokeBlocker->Destroy();
		SpawnedSmokeBlocker = nullptr;
	}
	
	Super::Destroyed();
}
