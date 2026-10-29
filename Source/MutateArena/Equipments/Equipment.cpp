#include "Equipment.h"

#include "AbilitySystemComponent.h"
#include "AnimInstEquipment.h"
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

	DOREPLIFETIME_CONDITION(AEquipment, EquipmentName, COND_InitialOnly);
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
    
	// 初始赋值 EquipmentAsset 并强制校验有效
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	check(GameInstance);

	if (!AssetSubsystem) AssetSubsystem = GameInstance->GetSubsystem<UAssetSubsystem>();
	check(AssetSubsystem);
    
	if (!EquipmentAsset) EquipmentAsset = AssetSubsystem->GetEquipmentAssetByName(EquipmentName);
	check(EquipmentAsset);

	// 应用皮肤材质覆盖
	for (int32 i = 0; i < EquipmentAsset->OverrideMaterials.Num(); ++i)
	{
		if (!EquipmentAsset->OverrideMaterials[i].IsNull())
		{
			if (UMaterialInterface* Mat = UDataAssetManager::Get().GetAsset(EquipmentAsset->OverrideMaterials[i]))
			{
				if (EquipmentMesh) EquipmentMesh->SetMaterial(i, Mat);
				if (EquipmentMesh1P) EquipmentMesh1P->SetMaterial(i, Mat);
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

UAnimInstEquipment* AEquipment::GetEquipmentAnimInst1P()
{
	if (AnimInstEquipment1P == nullptr) AnimInstEquipment1P = Cast<UAnimInstEquipment>(EquipmentMesh1P->GetAnimInstance());
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

void AEquipment::OnGiven(AHumanCharacter* HumanChar)
{
	if (!HumanChar) return;

	EquipmentState = EEquipmentState::Equipped;

	if (HumanChar->HasAuthority())
	{
		SetReplicateMovement(false);
	}

	SetOwner(HumanChar);
	SetOwnerTeam();

	// 清除延迟开启拾取碰撞定时器与销毁定时器
	GetWorldTimerManager().ClearTimer(AreaSphereTimerHandle);
	GetWorldTimerManager().ClearTimer(DestroyEquipmentTimerHandle);

	// 关闭地面物理与拾取碰撞
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
    
	// 延迟开启AreaSphere碰撞，确保武器已被丢出当前角色的Overlap区域
	GetWorldTimerManager().SetTimer(AreaSphereTimerHandle, this, &ThisClass::SetAreaSphereCollision, .4f, false);

	// 仅在服务端启动丢弃销毁定时器
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(DestroyEquipmentTimerHandle, this, &ThisClass::DestroyEquipment, 15.f, false);
	}
    
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

const UEquipmentAsset* AEquipment::EnsureEquipmentAsset()
{
	if (!EquipmentAsset && EquipmentName != EEquipmentName::None)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (!AssetSubsystem) AssetSubsystem = GameInstance->GetSubsystem<UAssetSubsystem>();
			if (AssetSubsystem)
			{
				EquipmentAsset = AssetSubsystem->GetEquipmentAssetByName(EquipmentName);
			}
		}
	}
	return EquipmentAsset;
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
