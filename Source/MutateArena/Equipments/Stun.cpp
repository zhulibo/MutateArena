#include "Stun.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MetaSoundSource.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"

AStun::AStun()
{
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.6f;
}

void AStun::ThrowOut()
{
	Super::ThrowOut();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::ServerExplode, 2.f);
	}
}

void AStun::ServerExplode()
{
	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	
	if (OwnerTeam == ETeam::NoTeam) SetOwnerTeam();

	// 寻找影响范围内的目标
	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState)
	{
		TArray<ABasePlayerState*> PlayerStates;
		BaseGameState->GetPlayerStates(OwnerTeam, PlayerStates);
		for (int32 i = 0; i < PlayerStates.Num(); ++i)
		{
			if (PlayerStates[i])
			{
				AActor* TeamActor = PlayerStates[i]->GetPawn();
				if (TeamActor && TeamActor != HumanCharacter)
				{
					ActorsToIgnore.AddUnique(TeamActor);
				}
			}
		}
	}
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(
		this,
		GetActorLocation(),
		Radius,
		ObjectTypes,
		ABaseCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappingActors
	);

	// 对OverlappingActors进行墙体阻挡检测
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // 忽略震撼弹自身
	if (BaseGameState)
	{
		TArray<ABasePlayerState*> PlayerStates;
		BaseGameState->GetPlayerStates(OwnerTeam, PlayerStates);
		for (APlayerState* PS : PlayerStates)
		{
			if (PS && PS->GetPawn())
			{
				QueryParams.AddIgnoredActor(PS->GetPawn());
			}
		}
		QueryParams.AddIgnoredActors(BaseGameState->AllEquipments);
	}

	for (AActor* Actor : OverlappingActors)
	{
		if (ABaseCharacter* TargetChar = Cast<ABaseCharacter>(Actor))
		{
			FVector TargetTraceLocation = TargetChar->GetActorLocation();
			if (TargetChar->Camera)
			{
				TargetTraceLocation = TargetChar->Camera->GetComponentLocation();
			}
			
			FHitResult HitResult;
			bool bHitWall = GetWorld()->LineTraceSingleByChannel(
				HitResult,
				GetActorLocation(),
				TargetTraceLocation,
				ECollisionChannel::ECC_Visibility,
				QueryParams
			);

			if (bHitWall) continue;

			// 视线无阻挡，应用震撼弹 GE
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetChar))
			{
				if (StunEffectClass)
				{
					FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
					ContextHandle.AddInstigator(HumanCharacter, this);
					TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
				}
			}
		}
	}

	// 广播特效和震动
	MulticastExplodeEffects();

	SetLifeSpan(2.0f);
}

void AStun::MulticastExplodeEffects_Implementation()
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

	// 播放世界范围的屏幕晃动，引擎会根据距离自动衰减
	if (CameraShakeClass)
	{
		UGameplayStatics::PlayWorldCameraShake(
			GetWorld(),
			CameraShakeClass,
			GetActorLocation(),
			0.f,
			Radius,
			1.f
		);
	}
}
