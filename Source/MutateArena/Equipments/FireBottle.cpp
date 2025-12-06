#include "FireBottle.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Data/DamageTypeEquipment.h"

AFireBottle::AFireBottle()
{
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	
	FireSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SmokeSphere"));
	FireSphere->SetupAttachment(RootComponent);
	FireSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireSphere->SetSphereRadius(FireRadius);
	
	ProjectileMovement->Bounciness = 0.1f;
	ProjectileMovement->Friction = 0.9f;
}

void AFireBottle::ThrowOut()
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
}

void AFireBottle::BeginPlay()
{
	Super::BeginPlay();
	
	CollisionSphere->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
}

void AFireBottle::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority())
	{
		ProjectileMovement->StopMovementImmediately();
		
		MulticastOnHit();
	}
}

void AFireBottle::MulticastOnHit_Implementation()
{
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	EquipmentMesh->SetHiddenInGame(true);
	
	Explode();
}

void AFireBottle::Explode()
{
	float Time = 10.f;

	SetLifeSpan(Time);

	auto ExplodeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplodeEffect,
		GetActorLocation(),
		FRotator(0.f, 0.f, 0.f)
	);
	if (ExplodeEffectComponent)
	{
		ExplodeEffectComponent->SetVariableFloat(TEXT("FireRadius"), FireRadius);
		ExplodeEffectComponent->SetVariableFloat(TEXT("Time"), Time);
	}

	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}

	AudioComponent = UGameplayStatics::SpawnSoundAttached(
		BurnSound,
		GetRootComponent(),
		NAME_None,
		GetActorLocation(),
		EAttachLocation::KeepWorldPosition
	);

	FireSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	GetWorldTimerManager().SetTimer(DetectTimerHandle, this, &ThisClass::DetectActors, 1.f, true);
}

void AFireBottle::DetectActors()
{
	if (BaseController == nullptr)
	{
		if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
		if (HumanCharacter) BaseController = Cast<ABaseController>(HumanCharacter->GetController());
	}
	if (BaseController)
	{
		TArray<AActor*> OverlappingActors;
		FireSphere->GetOverlappingActors(OverlappingActors, AActor::StaticClass());
		for (AActor* OverlapActor : OverlappingActors)
		{
			if (OverlapActor == nullptr) continue;
			
			// 烟雾可以灭火
			if (OverlapActor->ActorHasTag(TAG_SMOKE_ACTOR))
			{
				if (ExtinguishSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, ExtinguishSound, GetActorLocation());
				}

				Destroy();
			}
			else if (HasAuthority())
			{
				if (OverlapActor->ActorHasTag(TAG_CHARACTER_MUTANT))
				{
					if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OverlapActor))
					{
						UGameplayStatics::ApplyDamage(
							BaseCharacter,
							BaseCharacter->GetMaxHealth() * 0.05f,
							BaseController,
							this,
							UDamageTypeEquipment::StaticClass()
						);
					}
				}
				else if (OverlapActor->ActorHasTag(TAG_CHARACTER_HUMAN))
				{
					if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OverlapActor))
					{
						UGameplayStatics::ApplyDamage(
							BaseCharacter,
							BaseCharacter->GetMaxHealth() * 0.02f,
							BaseController,
							this,
							UDamageTypeEquipment::StaticClass()
						);
					}
				}
			}
		}
	}
}

void AFireBottle::Destroyed()
{
	GetWorldTimerManager().ClearTimer(DetectTimerHandle);

	// GC应该足够智能，播放完后会回收AudioComponent
	if (AudioComponent)
	{
		AudioComponent->FadeOut(1.f, 0.f);
	}

	Super::Destroyed();
}
