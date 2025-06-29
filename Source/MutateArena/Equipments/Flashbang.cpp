#include "Flashbang.h"
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

AFlashbang::AFlashbang()
{
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.6f;
}

void AFlashbang::ThrowOut()
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
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::Explode, 2.f);
}

void AFlashbang::Explode()
{
	// 不能直接销毁，同步到模拟端无法执行
	SetLifeSpan(1.f);

	if (BaseController == nullptr)
	{
		if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
		if (HumanCharacter) BaseController = Cast<ABaseController>(HumanCharacter->GetController());
	}

	if (ExplodeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	}

	ExplodeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		ExplodeEffect,
		GetActorLocation(),
		GetActorRotation()
	);

	// 检测周围玩家
	TArray<AActor*> DamagedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		Radius,
		ObjectTypes,
		ABaseCharacter::StaticClass(),
		TArray<AActor*>(),
		DamagedActors
	);

	for (AActor* Actor : DamagedActors)
	{
		if (ABaseCharacter* DamagedCharacter = Cast<ABaseCharacter>(Actor))
		{
			if (!DamagedCharacter->IsLocallyControlled()) continue;

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
				DamagedCharacter->Camera->GetComponentLocation(),
				ECollisionChannel::ECC_Visibility,
				QueryParams
			);
			
			if (bHit)
			{
				continue;
			}
			
			// 应用闪光
			float Distance = DamagedCharacter->GetDistanceTo(this);
			
			FVector A = DamagedCharacter->Camera->GetForwardVector();
			FVector B = GetActorLocation() - DamagedCharacter->Camera->GetComponentLocation();
			A.Normalize();
			B.Normalize();
			float DotProduct = FVector::DotProduct(A, B);
			DotProduct = FMath::Clamp(DotProduct, -1.f, 1.f);
			float AngleInRadians = FMath::Acos(DotProduct);
			float Angle = FMath::RadiansToDegrees(AngleInRadians);

			// if (DamagedCharacter->HasAuthority())
			// {
			// 	if (DamagedCharacter->IsLocallyControlled())
			// 	{
			// 		UE_LOG(LogTemp, Warning, TEXT("Server Angle %f"), Angle);
			// 	}
			// 	else
			// 	{
			// 		UE_LOG(LogTemp, Warning, TEXT("Angle %f"), Angle);
			// 	}
			// }

			if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
			if (AssetSubsystem && AssetSubsystem->CharacterAsset)
			{
				if (UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(AssetSubsystem->CharacterAsset->MPC_Flashbang))
				{
					MPCI->SetScalarParameterValue(FName("Radius"), Radius);
					MPCI->SetScalarParameterValue(FName("MaxFlashTime"), MaxFlashTime);
					MPCI->SetScalarParameterValue(FName("MaxCapTime"), MaxCapTime);
					MPCI->SetScalarParameterValue(FName("FlashTime"), GetWorld()->GetTimeSeconds());
					MPCI->SetScalarParameterValue(FName("Distance"), Distance);
					MPCI->SetScalarParameterValue(FName("Angle"), Angle);
				}
			}

			// 隐藏OverheadWidget
			float Speed = 1 / (FMath::Clamp(Distance / Radius, .5f, 1.f) * MaxCapTime);
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

			GetWorldTimerManager().SetTimerForNextTick([this, DamagedCharacter]() {
				DamagedCharacter->SceneCapture->CaptureScene();
			});
		}
	}
}
