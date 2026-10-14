#include "Equipment.h"

#include "AbilitySystemComponent.h"
#include "AnimInstEquipment.h"
#include "AnimInstEquipment1P.h"
#include "GameplayAbilitySpec.h"
#include "TimerManager.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Animation/AnimationAsset.h"
#include "MutateArena/MutateArena.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Data/EquipmentAsset.h"
#include "Engine/GameInstance.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"

AEquipment::AEquipment()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore); // 忽略角色胶囊体
	CollisionSphere->SetCollisionResponseToChannel(ECC::MeshTeam1, ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC::MeshTeam2, ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetLinearDamping(1.f);
	CollisionSphere->SetSphereRadius(20.f); // 离地面的距离

	EquipmentMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EquipmentMesh"));
	EquipmentMesh->SetupAttachment(RootComponent);
	EquipmentMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EquipmentMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	EquipmentMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	EquipmentMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block); // 与血液碰撞
	EquipmentMesh->SetOwnerNoSee(true);
	EquipmentMesh->bCastHiddenShadow = true;

	EquipmentMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EquipmentMesh1P"));
	EquipmentMesh1P->SetupAttachment(RootComponent);
	EquipmentMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquipmentMesh1P->SetOnlyOwnerSee(true);
	EquipmentMesh1P->CastShadow = false;
	EquipmentMesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->SetupAttachment(RootComponent);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	OverlapSphere->SetSphereRadius(50.f);
	OverlapSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnAreaSphereOverlap);

	OwnerTeam = ETeam::NoTeam;

	Tags.Add(Tags::Equipment);
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
    
    if (UWorld* World = GetWorld())
    {
       if (ABaseGameState* GameState = World->GetGameState<ABaseGameState>())
       {
          GameState->AddEquipment(this);
       }
    }
    
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (GameInstance == nullptr) return;

    if (!AssetSubsystem) AssetSubsystem = GameInstance->GetSubsystem<UAssetSubsystem>();
    if (AssetSubsystem)
    {
       // 1. 默认当前武器就是基础武器
       EquipmentParentName = EquipmentName;

       // 2. 检查当前是否为皮肤。如果是，则更新 ParentName 为皮肤对应的基础武器
       UEquipmentSkinAsset* SkinAsset = AssetSubsystem->GetEquipmentSkinAssetByName(EquipmentName);
       if (SkinAsset)
       {
          EquipmentParentName = SkinAsset->EquipmentParentName;
       }

       // 3. 读取基础武器的数据 (无论是本体还是皮肤，Cate和Type都由基础武器决定)
       if (UEquipmentAsset* BaseAsset = AssetSubsystem->GetEquipmentAssetByName(EquipmentParentName))
       {
          EquipmentCate = BaseAsset->EquipmentCate;
          EquipmentType = BaseAsset->EquipmentType;

          // 4. 屏蔽皮肤逻辑
          bool bIsMyOwnWeapon = false;
          if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
          {
             if (OwningPawn->IsLocallyControlled()) bIsMyOwnWeapon = true;
          }
          
          if (StorageSubsystem == nullptr) StorageSubsystem = GameInstance->GetSubsystem<UStorageSubsystem>();
          if (StorageSubsystem && StorageSubsystem->CacheSetting)
          {
             // 如果开启了屏蔽皮肤功能，且这不是自己的武器，并且当前确实是皮肤(SkinAsset有效)
             if (StorageSubsystem->CacheSetting->bHideSkins && !bIsMyOwnWeapon && SkinAsset)
             {
                // 使用基础武器的默认模型来覆盖皮肤模型
                if (USkeletalMesh* DefaultSkeletalMesh = UDataAssetManager::Get().GetAsset(BaseAsset->DefaultSkeletalMesh))
                {
                   EquipmentMesh->SetSkeletalMesh(DefaultSkeletalMesh);
                   EquipmentMesh->EmptyOverrideMaterials();
                }
             }
          }
       }
    }
}

void AEquipment::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ABaseGameState* GameState = World->GetGameState<ABaseGameState>())
		{
			GameState->RemoveEquipment(this);
		}
	}
	
	Super::EndPlay(EndPlayReason);
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
	if (AnimInstEquipment == nullptr) AnimInstEquipment = Cast<UAnimInstEquipment>(EquipmentMesh->GetAnimInstance());
	return AnimInstEquipment;
}

UAnimInstEquipment1P* AEquipment::GetEquipmentAnimInst1P()
{
	if (AnimInstEquipment1P == nullptr) AnimInstEquipment1P = Cast<UAnimInstEquipment1P>(EquipmentMesh1P->GetAnimInstance());
	return AnimInstEquipment1P;
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

void AEquipment::OnGiven(class AHumanCharacter* HumanChar)
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
	
	if (EquipmentMesh)
	{
		EquipmentMesh->SetOwnerNoSee(true);
		EquipmentMesh->SetVisibility(true);
	}
	if (EquipmentMesh1P)
	{
		EquipmentMesh1P->SetVisibility(true);
	}
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
	
	if (EquipmentMesh1P)
	{
		EquipmentMesh1P->SetVisibility(false);
	}
	if (EquipmentMesh)
	{
		EquipmentMesh->SetOwnerNoSee(false);
		EquipmentMesh->SetVisibility(true);
	}
	
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
