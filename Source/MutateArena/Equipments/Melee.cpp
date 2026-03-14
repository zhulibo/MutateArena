#include "Melee.h"

#include "DataRegistrySubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Effects/BloodCollision.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/CapsuleComponent.h"
#include "Data/DamageTypeMelee.h"
#include "Kismet/GameplayStatics.h"
#include "Herbs/Herb.h"
#include "Net/UnrealNetwork.h"

AMelee::AMelee()
{
	AttackCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("AttackCapsule"));
	AttackCapsule->SetupAttachment(RootComponent);
	AttackCapsule->SetGenerateOverlapEvents(true);
	AttackCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AttackCapsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnAttackCapsuleOverlap);
}

void AMelee::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bIsLightAttack);
}

void AMelee::BeginPlay()
{
	Super::BeginPlay();

	FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(EquipmentParentName));
	FDataRegistryId DataRegistryId(DR_MELEE_DATA, FName(EnumValue));
	if (const FMeleeData* MeleeData = UDataRegistrySubsystem::Get()->GetCachedItem<FMeleeData>(DataRegistryId))
	{
		LightAttackDamage = MeleeData->LightAttackDamage;
		HeavyAttackDamage = MeleeData->HeavyAttackDamage;
		WalkSpeedMul = MeleeData->WalkSpeedMul;
	}

	SetAttackCapsuleCollision();
}

void AMelee::OnEquip(class AHumanCharacter* HumanChar)
{
	Super::OnEquip(HumanChar);

	SetAttackCapsuleCollision();
}

void AMelee::SetAttackCapsuleCollision()
{
	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		switch (OwnerTeam)
		{
		case ETeam::Team1:
			AttackCapsule->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Overlap);
			break;
		case ETeam::Team2:
			AttackCapsule->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Overlap);
			break;
		}

		AttackCapsule->SetCollisionResponseToChannel(ECC_MESH_HREB, ECollisionResponse::ECR_Overlap);
	}
}

void AMelee::OnStartSwapOut()
{
	Super::OnStartSwapOut();

	SetAttackCollisionEnabled(false);
}

void AMelee::SetAttackCollisionEnabled(bool bIsEnabled)
{
	if (bIsEnabled)
	{
		AttackCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		AttackCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMelee::OnAttackCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// UE_LOG(LogTemp, Warning, TEXT("OnAttackCapsuleOverlap %d"), GetLocalRole());

	AHumanCharacter* InstigatorCharacter = Cast<AHumanCharacter>(GetOwner());
	if (InstigatorCharacter && !HitEnemies.Contains(OtherActor))
	{
		HitEnemies.Add(OtherActor);

		if (OtherActor->ActorHasTag(TAG_CHARACTER_BASE))
		{
			float Damage = bIsLightAttack ? LightAttackDamage : HeavyAttackDamage;
			
			DropBlood(OverlappedComponent, OtherActor, OtherComp, Damage);
		
			if (InstigatorCharacter->IsLocallyControlled())
			{
				ServerApplyDamage(OtherActor, InstigatorCharacter, Damage);
			}
		}
		else if (OtherActor->ActorHasTag(TAG_HERB))
		{
			if (AHerb* Herb = Cast<AHerb>(OtherActor))
			{
				Herb->ServerDestroy();
			}
		}
	}
}

void AMelee::ServerApplyDamage_Implementation(AActor* OtherActor, AHumanCharacter* InstigatorCharacter, float Damage)
{
	if (InstigatorCharacter)
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, InstigatorCharacter->Controller, this, UDamageTypeMelee::StaticClass());
	}
}

void AMelee::ClearHitEnemies()
{
	HitEnemies.Empty();
}

void AMelee::DropBlood(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, float Damage)
{
	ABaseCharacter* OverlappedCharacter = Cast<ABaseCharacter>(OtherActor);

	if (OverlappedCharacter == nullptr || OverlappedCharacter->BloodEffect == nullptr) return;

	TArray<FHitResult> TraceResults;

	auto Start = OverlappedComponent->GetComponentLocation();
	auto End = OverlappedCharacter->GetActorLocation();

	GetWorld()->SweepMultiByObjectType(
		TraceResults,
		Start,
		End,
		FQuat::Identity,
		FCollisionObjectQueryParams(OverlappedCharacter->GetMesh()->GetCollisionObjectType()),
		FCollisionShape::MakeSphere(10.f)
	);

	// DrawDebugLine(GetWorld(), Start, End, C_YELLOW, true);

	for (auto TraceResult : TraceResults)
	{
		if (TraceResult.GetComponent()->GetUniqueID() == OtherComp->GetUniqueID())
		{
			auto BloodEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				OverlappedCharacter->BloodEffect,
				TraceResult.ImpactPoint,
				TraceResult.ImpactNormal.Rotation()
			);
			if (BloodEffectComponent)
			{
				BloodEffectComponent->SetVariableInt(TEXT("Count"), ULibraryCommon::GetBloodParticleCount(Damage));
				BloodEffectComponent->SetVariableLinearColor(TEXT("Color"), OverlappedCharacter->BloodColor);
				UBloodCollision* CollisionCB = NewObject<UBloodCollision>(this);
				BloodEffectComponent->SetVariableObject(TEXT("CollisionCB"), CollisionCB);
			}
			
			auto BloodSmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				OverlappedCharacter->BloodSmokeEffect,
				TraceResult.ImpactPoint,
				TraceResult.ImpactNormal.Rotation()
			);
			if (BloodSmokeEffectComponent)
			{
				BloodSmokeEffectComponent->SetVariableLinearColor(TEXT("SmokeColor"), OverlappedCharacter->BloodColor);
			}

			break;
		}
	}
}
