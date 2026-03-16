#include "Equipment.h"

#include "AbilitySystemComponent.h"
#include "DataRegistrySubsystem.h"
#include "AnimInstEquipment.h"
#include "GameplayAbilitySpec.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Animation/AnimationAsset.h"
#include "MutateArena/MutateArena.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

AEquipment::AEquipment()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore); // 忽略角色胶囊体
	CollisionSphere->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetLinearDamping(1.f);
	CollisionSphere->SetSphereRadius(20.f); // 离地面的距离

	EquipmentMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EquipmentMesh"));
	EquipmentMesh->SetupAttachment(RootComponent);
	EquipmentMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EquipmentMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	EquipmentMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	EquipmentMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block); // 与血液碰撞

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	OverlapSphere->SetSphereRadius(50.f);
	OverlapSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnAreaSphereOverlap);

	OwnerTeam = ETeam::NoTeam;

	Tags.Add(TAG_EQUIPMENT);
}

void AEquipment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, OwnerTeam);
	DOREPLIFETIME(ThisClass, bIsHidden);
	DOREPLIFETIME(ThisClass, EquipmentState);
}

void AEquipment::BeginPlay()
{
	Super::BeginPlay();

	FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(EquipmentName));
	FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
	if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
	{
		EquipmentParentName = EquipmentMain->EquipmentParentName;
		EquipmentCate = EquipmentMain->EquipmentCate;
		EquipmentType = EquipmentMain->EquipmentType;

		// TODO 待测试屏蔽皮肤
		// 屏蔽皮肤
		bool bIsMyOwnWeapon = false;
		if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
		{
			if (OwningPawn->IsLocallyControlled()) bIsMyOwnWeapon = true;
		}
		if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem->CacheSetting->bHideSkins && !bIsMyOwnWeapon && EquipmentName != EquipmentParentName)
		{
			FString EnumValue2 = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(EquipmentParentName));
			FDataRegistryId DataRegistryId2(DR_EQUIPMENT_MAIN, FName(EnumValue2));
			if (const FEquipmentMain* EquipmentMain2 = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId2))
			{
				USkeletalMesh* DefaultSkeletalMesh = UDataAssetManager::Get().GetAsset(EquipmentMain2->DefaultSkeletalMesh);
				EquipmentMesh->SetSkeletalMesh(DefaultSkeletalMesh);
			}
		}
	}
}

void AEquipment::OnAreaSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (AHumanCharacter* OverlapHumanCharacter = Cast<AHumanCharacter>(OtherActor))
	{
		OverlapHumanCharacter->EquipOverlappingEquipment_Server(this);
	}
}

UAnimInstEquipment* AEquipment::GetEquipmentAnimInst()
{
	if (EquipmentAnimInst == nullptr) EquipmentAnimInst = Cast<UAnimInstEquipment>(EquipmentMesh->GetAnimInstance());
	return EquipmentAnimInst;
}

void AEquipment::SetHiddenMesh(bool TempbIsHidden)
{
	bIsHidden = TempbIsHidden;

	OnRep_IsHidden(); 
}

void AEquipment::OnRep_IsHidden()
{
	if (EquipmentMesh)
	{
		EquipmentMesh->SetVisibility(!bIsHidden);
	}
}

void AEquipment::OnEquip(class AHumanCharacter* HumanChar)
{
	EquipmentState = EEquipmentState::Equipped;

	if (HumanChar->HasAuthority())
	{
		SetReplicateMovement(false);
		
		SetOwner(HumanChar);
	}

	SetOwnerTeam();
	
	// 清除销毁定时器
	GetWorldTimerManager().ClearTimer(DestroyEquipmentTimerHandle);

	CollisionSphere->SetSimulatePhysics(false);
	CollisionSphere->SetEnableGravity(false);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEquipment::Drop()
{
	EquipmentState = EEquipmentState::Dropped;

	if (HasAuthority())
	{
		SetReplicateMovement(true);
	}
	
	HandleDrop();
}


void AEquipment::OnRep_EquipmentState(EEquipmentState OldState)
{
	if (EquipmentState == EEquipmentState::Dropped)
	{
		HandleDrop();
	}
	else if (EquipmentState == EEquipmentState::Equipped)
	{
		// OnEquip 在所有端都调用了，这里不用处理
	}
}

void AEquipment::HandleDrop()
{
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	CollisionSphere->DetachFromComponent(DetachRules);
	
	// 重置旋转，确保掉落时始终朝上平躺
	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Pitch = 0.f;
	CurrentRotation.Roll = 0.f;
	SetActorRotation(CurrentRotation);
	
	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetEnableGravity(true);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// 丢弃武器时，给予一个向前的冲量
	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		if (UCameraComponent* CameraComponent = HumanCharacter->FindComponentByClass<UCameraComponent>())
		{
			float Impulse = HumanCharacter->bIsDead ? 100.f : 400.f;
			CollisionSphere->AddImpulse(CameraComponent->GetForwardVector() * Impulse, NAME_None, true);
		}
	}
	
	// HACK 延迟开启AreaSphere碰撞 确保武器已被丢出当前角色的Overlap区域
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::SetAreaSphereCollision, .4f);

	// 丢弃一定时间后销毁
	GetWorldTimerManager().SetTimer(DestroyEquipmentTimerHandle, this, &ThisClass::DestroyEquipment, 15.f);
	
	SetOwner(nullptr);
	OwnerTeam = ETeam::NoTeam;
}

void AEquipment::SetOwnerTeam()
{
	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		if (ABasePlayerState* PlayerState = Cast<ABasePlayerState>(HumanCharacter->GetPlayerState()))
		{
			OwnerTeam = PlayerState->Team;
		}
	}
}

void AEquipment::SetAreaSphereCollision()
{
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEquipment::DestroyEquipment()
{
	Destroy();
}

void AEquipment::GiveAbilities(UAbilitySystemComponent* ASC)
{
	if (!ASC || !HasAuthority()) return; 

	for (TSubclassOf<UGameplayAbility> AbilityClass :Abilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
            
			FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
			GrantedAbilityHandles.Add(Handle);
		}
	}
}

void AEquipment::ClearAbilities(UAbilitySystemComponent* ASC)
{
	if (!ASC || !HasAuthority()) return;

	for (FGameplayAbilitySpecHandle Handle : GrantedAbilityHandles)
	{
		ASC->ClearAbility(Handle);
	}
    
	GrantedAbilityHandles.Empty();
}
