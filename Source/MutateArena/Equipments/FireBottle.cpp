#include "FireBottle.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "MutateArena/Equipments/Data/DamageTypeEquipment.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/Maps/Environment/EnvironmentManager.h"
#include "MutateArena/System/GameplayTags.h"

AFireBottle::AFireBottle()
{
	CollisionSphere->SetNotifyRigidBodyCollision(true);

	FireSphere = CreateDefaultSubobject<USphereComponent>(TEXT("FireSphere")); // 修改了这里的名字以防跟SmokeSphere混淆
	FireSphere->SetupAttachment(RootComponent);
	FireSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireSphere->SetSphereRadius(FireRadius);
	
	// ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->Bounciness = 0.1f;
	ProjectileMovement->Friction = 0.9f;
}

void AFireBottle::ThrowOut()
{
	Super::ThrowOut();
}

void AFireBottle::BeginPlay()
{
	Super::BeginPlay();

	// 只有服务器需要关心碰撞逻辑来触发爆炸
	if (HasAuthority())
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
}

void AFireBottle::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority() && !bHasExploded)
	{
		bHasExploded = true;

		MulticastExplodeEffects();

		FireSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		
		// 开启服务器伤害循环定时器
		GetWorldTimerManager().SetTimer(DetectTimerHandle, this, &ThisClass::ServerDetectActors, 1.f, true);

		SetLifeSpan(Time);
	}
}

void AFireBottle::MulticastExplodeEffects_Implementation()
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}

	SpawnedFireEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplodeEffect,
		GetActorLocation(),
		FRotator::ZeroRotator
	);
	if (SpawnedFireEffect)
	{
		SpawnedFireEffect->SetVariableFloat(TEXT("FireRadius"), FireRadius);
		SpawnedFireEffect->SetVariableFloat(TEXT("Time"), Time);
	}

	SpawnedFireSound = UGameplayStatics::SpawnSoundAttached(
		BurnSound,
		GetRootComponent(),
		NAME_None,
		GetActorLocation(),
		EAttachLocation::KeepWorldPosition
	);
}

void AFireBottle::ServerDetectActors()
{
	if (!HasAuthority()) return;

	TArray<AActor*> OverlappingActors;
	FireSphere->GetOverlappingActors(OverlappingActors, AActor::StaticClass());

	// 烟雾灭火
	for (AActor* OverlapActor : OverlappingActors)
	{
		if (OverlapActor && OverlapActor->ActorHasTag(Tags::SmokeActor))
		{
			MulticastExtinguishEffects();
			
			SetLifeSpan(0.5f);
			return;
		}
	}
	
	// 酸雨灭火
	if (ABaseGameState* GameState = GetWorld()->GetGameState<ABaseGameState>())
	{
		if (GameState->EnvironmentManager && GameState->EnvironmentManager->IsEnvironmentActive(TAG_STATE_ENV_ACIDRAIN))
		{
			FVector StartLoc = GetActorLocation();
			FVector EndLoc = StartLoc + FVector(0.0f, 0.0f, 10000.0f);

			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);

			// 使用世界静态通道判断上方是否有遮挡物 (屋顶等)
			bool bIsIndoors = GetWorld()->LineTraceSingleByObjectType(
				HitResult,
				StartLoc,
				EndLoc,
				FCollisionObjectQueryParams(ECC_WorldStatic), 
				QueryParams
			);

			if (!bIsIndoors)
			{
				MulticastExtinguishEffects();
				SetLifeSpan(0.5f);
				return;
			}
		}
	}
	
	// 赋予火焰伤害
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	ABaseController* BaseController = HumanCharacter ? Cast<ABaseController>(HumanCharacter->GetController()) : nullptr;
	for (AActor* OverlapActor : OverlappingActors)
	{
		if (OverlapActor == nullptr) continue;

		if (OverlapActor->ActorHasTag(Tags::CharacterMutant))
		{
			if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OverlapActor))
			{
				UGameplayStatics::ApplyDamage(
					BaseCharacter,
					BaseCharacter->GetMaxHealth() * 0.05f,
					BaseController, // 即使 Controller 为空，ApplyDamage 也允许执行环境伤害
					this,
					UDamageTypeEquipment::StaticClass()
				);
			}
		}
		else if (OverlapActor->ActorHasTag(Tags::CharacterHuman))
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

void AFireBottle::MulticastExtinguishEffects_Implementation()
{
	if (ExtinguishSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExtinguishSound, GetActorLocation());
	}
}

void AFireBottle::Destroyed()
{
	// 清理定时器
	GetWorldTimerManager().ClearTimer(DetectTimerHandle);

	if (SpawnedFireEffect)
	{
		SpawnedFireEffect->DestroyComponent();
		SpawnedFireEffect = nullptr;
	}
	
	// 平滑淡出燃烧音效
	if (SpawnedFireSound)
	{
		SpawnedFireSound->FadeOut(1.f, 0.f);
	}

	Super::Destroyed();
}
