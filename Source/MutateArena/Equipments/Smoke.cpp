#include "Smoke.h"
#include "Data/EquipmentAsset.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Sub/SmokeBlocker.h"

ASmoke::ASmoke()
{
}

void ASmoke::BeginPlay()
{
	Super::BeginPlay();

	FragSmoke = EquipmentAsset->FindFragmentByClass<UFragSmoke>();
	check(FragSmoke);
}

void ASmoke::ThrowOut()
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

void ASmoke::ServerExplode()
{
	MulticastExplodeEffects();

	SetLifeSpan(FragSmoke->Duration);
}

void ASmoke::MulticastExplodeEffects_Implementation()
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	// 播放特效和声音
	if (FragThrowable->ExplodeEffect)
	{
		SpawnedSmokeEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			FragThrowable->ExplodeEffect,
			GetActorLocation(),
			GetActorRotation()
		);

		if (SpawnedSmokeEffect)
		{
			SpawnedSmokeEffect->SetVariableFloat(TEXT("Time"), FragSmoke->Duration);
		}
	}

	if (FragThrowable->ExplodeSound)
	{
		SpawnedSmokeSound = UGameplayStatics::SpawnSoundAtLocation(
			this,
			FragThrowable->ExplodeSound,
			GetActorLocation()
		);
	}
	
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
		SpawnedSmokeBlocker->SetLifeSpan(FragSmoke->Duration);
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
