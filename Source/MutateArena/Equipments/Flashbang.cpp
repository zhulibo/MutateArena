#include "Flashbang.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/Characters/Components/OverheadWidget.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/Tags/ProjectTags.h"

AFlashbang::AFlashbang()
{
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.6f;
}

void AFlashbang::ThrowOut()
{
	Super::ThrowOut();

	if (HasAuthority())
	{
		FTimerHandle TimerHandleSound;
		GetWorldTimerManager().SetTimer(TimerHandleSound, this, &ThisClass::ServerPlaySound, 1.8f);

		FTimerHandle TimerHandleExplode;
		GetWorldTimerManager().SetTimer(TimerHandleExplode, this, &ThisClass::ServerExplode, 2.0f);
	}
}

void AFlashbang::ServerPlaySound()
{
	MulticastPlaySound();
}

void AFlashbang::MulticastPlaySound_Implementation()
{
	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}
}

void AFlashbang::ServerExplode()
{
	MulticastExplode();

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	// 延迟销毁，保证多播RPC顺利到达且特效播放完毕
	SetLifeSpan(2.0f);
}

void AFlashbang::MulticastExplode_Implementation()
{
	// 播放闪光特效
	if (ExplodeEffect)
	{
		ExplodeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplodeEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	// 遍历本地玩家控制器
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC && PC->IsLocalController())
		{
			ABaseCharacter* LocalCharacter = Cast<ABaseCharacter>(PC->GetPawn());
			if (!LocalCharacter || !LocalCharacter->Camera) continue;

			// 仅当该本地玩家在爆炸半径内时，才进行检测和应用
			float Distance = LocalCharacter->GetDistanceTo(this);
					
			float FinalRadius = Radius;
			float FinalMaxFlashTime = MaxFlashTime;
			float FinalMaxCapTime = MaxCapTime;
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(LocalCharacter))
			{
				if (ASC->HasMatchingGameplayTag(TAG_STATE_DNA_EnhancedVision))
				{
					FinalRadius *= 1.1f; 
					FinalMaxFlashTime *= 1.1f; 
					FinalMaxCapTime *= 1.1f;
				}
			}
			
			if (Distance <= FinalRadius)
			{
				// 射线检测是否有阻挡
				FHitResult HitResult;
				FCollisionQueryParams QueryParams;

				TArray<AActor*> AllPlayers;
				UGameplayStatics::GetAllActorsWithTag(GetWorld(), TAG_CHARACTER_BASE, AllPlayers);
				QueryParams.AddIgnoredActors(AllPlayers);

				TArray<AActor*> AllEquipments;
				UGameplayStatics::GetAllActorsWithTag(GetWorld(), TAG_EQUIPMENT, AllEquipments);
				QueryParams.AddIgnoredActors(AllEquipments);

				bool bHit = GetWorld()->LineTraceSingleByChannel(
					HitResult,
					GetActorLocation(),
					LocalCharacter->Camera->GetComponentLocation(),
					ECollisionChannel::ECC_Visibility,
					QueryParams
				);

				// 如果没有阻挡，才被致盲
				if (!bHit)
				{
					FVector A = LocalCharacter->Camera->GetForwardVector();
					FVector B = GetActorLocation() - LocalCharacter->Camera->GetComponentLocation();
					A.Normalize();
					B.Normalize();
					float DotProduct = FVector::DotProduct(A, B);
					DotProduct = FMath::Clamp(DotProduct, -1.f, 1.f);
					float AngleInRadians = FMath::Acos(DotProduct);
					float Angle = FMath::RadiansToDegrees(AngleInRadians);
					
					UAssetSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
					if (Subsystem && Subsystem->CharacterAsset)
					{
						if (UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(
							Subsystem->CharacterAsset->MPC_Flashbang))
						{
							MPCI->SetScalarParameterValue(FName("Radius"), FinalRadius);
							MPCI->SetScalarParameterValue(FName("MaxFlashTime"), FinalMaxFlashTime);
							MPCI->SetScalarParameterValue(FName("MaxCapTime"), FinalMaxCapTime);
							MPCI->SetScalarParameterValue(FName("FlashTime"), GetWorld()->GetTimeSeconds());
							MPCI->SetScalarParameterValue(FName("Distance"), Distance);
							MPCI->SetScalarParameterValue(FName("Angle"), Angle);
						}
					}

					// 隐藏所有其他玩家的 OverheadWidget
					float Speed = 1 / (FMath::Clamp(1.0f - Distance / FinalRadius, .5f, 1.f) * FinalMaxCapTime);
					for (AActor* Player : AllPlayers)
					{
						if (ABaseCharacter* PlayerCharacter = Cast<ABaseCharacter>(Player))
						{
							if (UWidgetComponent* OverheadWidget = PlayerCharacter->OverheadWidget)
							{
								if (UOverheadWidget* OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject()))
								{
									OverheadWidgetClass->PlayFlashbangEffect(Speed);
								}
							}
						}
					}

					TWeakObjectPtr<AFlashbang> WeakThis = this;
					TWeakObjectPtr<ABaseCharacter> WeakCharacter = LocalCharacter;
					GetWorldTimerManager().SetTimerForNextTick([WeakThis, WeakCharacter]()
					{
						if (WeakThis.IsValid() && WeakCharacter.IsValid() && WeakCharacter->SceneCapture)
						{
							WeakCharacter->SceneCapture->CaptureScene();
						}
					});
				}
			}
		}
	}
}
