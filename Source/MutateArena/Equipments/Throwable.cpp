#include "Throwable.h"

#include "MetaSoundSource.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/EquipmentAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/System/AssetSubsystem.h"

AThrowable::AThrowable()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere->SetSphereRadius(10.f); // 离地面的距离

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->SetIsReplicated(true);
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->SetAutoActivate(false);
}

void AThrowable::BeginPlay()
{
	Super::BeginPlay();
	
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	check(AssetSubsystem);

	FragThrowable = EquipmentAsset->FindFragmentByClass<UFragThrowable>();
	check(FragThrowable);
	
	if (ProjectileMovement)
	{
		ProjectileMovement->OnProjectileBounce.AddDynamic(this, &AThrowable::OnProjectileBounce);
	}
}

void AThrowable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 投掷飞行中将因网络修正产生的 3P 网格体视觉偏移平滑插值归零，消除客户端回拉抽搐
	if (EquipmentState == EEquipmentState::Thrown && EquipmentMesh)
	{
		const FVector RelLoc = EquipmentMesh->GetRelativeLocation();
		const FRotator RelRot = EquipmentMesh->GetRelativeRotation();

		if (!RelLoc.IsNearlyZero(0.1f) || !RelRot.IsNearlyZero(0.1f))
		{
			EquipmentMesh->SetRelativeLocationAndRotation(
				FMath::VInterpTo(RelLoc, FVector::ZeroVector, DeltaTime, 10.f),
				FMath::RInterpTo(RelRot, FRotator::ZeroRotator, DeltaTime, 10.f)
			);
		}
		else if (!RelLoc.IsZero() || !RelRot.IsZero())
		{
			EquipmentMesh->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		}
	}
}

void AThrowable::PostNetReceiveLocationAndRotation()
{
	const bool bShouldSmooth = (EquipmentState == EEquipmentState::Thrown && bHasThrownOut && EquipmentMesh != nullptr);
	const FTransform PreMeshWorldTransform = bShouldSmooth ? EquipmentMesh->GetComponentTransform() : FTransform::Identity;

	Super::PostNetReceiveLocationAndRotation();

	// 物理根组件对齐服务端最新位置后，暂留视觉网格体的原世界变换，交由 Tick 平滑追赶
	if (bShouldSmooth)
	{
		if (FVector::DistSquared(PreMeshWorldTransform.GetLocation(), GetActorLocation()) < FMath::Square(300.f))
		{
			EquipmentMesh->SetWorldLocationAndRotation(
				PreMeshWorldTransform.GetLocation(),
				PreMeshWorldTransform.GetRotation()
			);
		}
	}
}

void AThrowable::ThrowOut()
{
	if (bHasThrownOut)
	{
		return;
	}
	bHasThrownOut = true;

	// 取本机当前可见的网格体作为出手点
	const FTransform StartTransform = GetVisibleEquipmentMesh()->GetComponentTransform();

	EquipmentState = EEquipmentState::Thrown;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 将 1P 和 3P 网格体从手上挂回根组件，随投掷物一起飞行
	EquipmentMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	EquipmentMesh1P->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	SetActorLocationAndRotation(StartTransform.GetLocation(), StartTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);

	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 飞行中所有端都显示 3P 网格体
	UpdateViewMode();

	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		if (HumanCharacter->HasAuthority() || HumanCharacter->IsLocallyControlled())
		{
			float ThrowSpeed = 1500.f;
			if (const UCharacterMovementComponent* MoveComp = HumanCharacter->GetCharacterMovement())
			{
				if (MoveComp->IsFalling() && !HumanCharacter->bIsCrouched)
				{
					// 跳跃/滞空且未按蹲：加大投掷力度（跳投）
					ThrowSpeed = 2000.f;
				}
				else if (MoveComp->IsMovingOnGround() && HumanCharacter->bIsCrouched)
				{
					// 地面蹲伏：减小投掷力度（近点轻抛）
					ThrowSpeed = 1000.f;
				}
			}

			FRotator ThrowRot = HumanCharacter->GetViewRotation();
			ThrowRot.Pitch = FMath::Clamp(FRotator::NormalizeAxis(ThrowRot.Pitch) + 10.f, -89.f, 89.f); // 抬高了点
			ProjectileMovement->Velocity = ThrowRot.Vector() * ThrowSpeed;
			ProjectileMovement->Activate();
		}
	}

	SetReplicateMovement(true);
}

void AThrowable::OnRep_EquipmentState(EEquipmentState OldState)
{
	Super::OnRep_EquipmentState(OldState);
	
	if (EquipmentState == EEquipmentState::Thrown && !bHasThrownOut)
	{
		// 主控端（含本地预测的客机与房主）已由 GA 直接触发 ThrowOut()，仅旁观代理端在此补触发
		const AHumanCharacter* HumanChar = Cast<AHumanCharacter>(GetOwner());
		if (!HumanChar || !HumanChar->IsLocallyControlled())
		{
			ThrowOut();
		}
	}
}

void AThrowable::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (!FragThrowable->HitSound || bHasBounced) return;

	bHasBounced = true;

	UGameplayStatics::SpawnSoundAttached(
		FragThrowable->HitSound, 
		GetRootComponent(), 
		NAME_None, 
		FVector::ZeroVector, 
		EAttachLocation::KeepRelativeOffset, 
		true
	);
}
