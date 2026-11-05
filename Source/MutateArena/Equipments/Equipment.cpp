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
	EquipmentMesh->CastShadow = true;
	EquipmentMesh->bCastHiddenShadow = false;

	EquipmentMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EquipmentMesh1P"));
	EquipmentMesh1P->SetupAttachment(RootComponent);
	EquipmentMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquipmentMesh1P->CastShadow = false;
	EquipmentMesh1P->SetVisibility(false);
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
	if (!AssetSubsystem) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	check(AssetSubsystem);
	
	EquipmentAsset = AssetSubsystem->GetEquipmentAssetByName(EquipmentName);
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

	UpdateViewMode();
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

void AEquipment::OnRep_Owner()
{
	Super::OnRep_Owner();

	// 客户端收到 Owner 同步时刷新一次视角状态，避免 Owner 与 bIsHidden 网络包先后到达引起的显隐偏差
	UpdateViewMode();
}

void AEquipment::OnAreaSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (AHumanCharacter* OverlapHumanCharacter = Cast<AHumanCharacter>(OtherActor))
	{
		OverlapHumanCharacter->EquipOverlappingEquipment(this);
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

void AEquipment::SetHiddenMesh(bool bInHidden)
{
	bIsHidden = bInHidden;

	UpdateViewMode();
}

void AEquipment::UpdateViewMode()
{
	// 1. 武器被切走（放入背包隐藏）状态：1P 和 3P 模型均不显示，且关闭隐藏阴影避免地面多出悬空枪影
	if (bIsHidden)
	{
		if (EquipmentMesh)
		{
			EquipmentMesh->SetVisibility(false);
			EquipmentMesh->bCastHiddenShadow = false;
			EquipmentMesh->MarkRenderStateDirty();
		}
		if (EquipmentMesh1P)
		{
			EquipmentMesh1P->SetVisibility(false);
		}
		return;
	}

	// 2. 武器掉落在地面（或尚未被拾取）状态：显示 3P 模型，关闭隐藏阴影，隐藏 1P 模型
	ABaseCharacter* OwnerChar = Cast<ABaseCharacter>(GetOwner());
	if (EquipmentState == EEquipmentState::Dropped || !OwnerChar)
	{
		if (EquipmentMesh)
		{
			EquipmentMesh->SetVisibility(true);
			EquipmentMesh->bCastHiddenShadow = false;
			EquipmentMesh->MarkRenderStateDirty();
		}
		if (EquipmentMesh1P)
		{
			EquipmentMesh1P->SetVisibility(false);
		}
		return;
	}

	// 3. 武器当前装备在角色手上（Equipped 且 !bIsHidden）：根据持有者是否为当前客户端的 ViewTarget 切换 1P/3P
	const bool bFirstPerson = OwnerChar->bIsCurrentViewTarget;

	if (EquipmentMesh)
	{
		// 1P 视角下隐藏 3P 武器，但开启 bCastHiddenShadow 让地面 MetaHuman 阴影手里有枪
		EquipmentMesh->SetVisibility(!bFirstPerson);
		EquipmentMesh->bCastHiddenShadow = bFirstPerson;
		EquipmentMesh->MarkRenderStateDirty();
	}

	if (EquipmentMesh1P)
	{
		EquipmentMesh1P->SetVisibility(bFirstPerson);
	}
}

void AEquipment::OnGiven(AHumanCharacter* HumanChar)
{
	if (!HumanChar) return;

	EquipmentState = EEquipmentState::Equipped;

	if (HumanChar->HasAuthority())
	{
		SetReplicateMovement(false);
		SetOwner(HumanChar);
		SetOwnerTeam();

		// 清除延迟开启拾取碰撞定时器与销毁定时器
		GetWorldTimerManager().ClearTimer(AreaSphereTimerHandle);
		GetWorldTimerManager().ClearTimer(DestroyEquipmentTimerHandle);
	}

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
		UpdateViewMode();
	}
}

void AEquipment::HandleDrop()
{
	bIsHidden = false;

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	CollisionSphere->DetachFromComponent(DetachRules);
	
	// 将之前在 AttachToHand 中分别挂到角色 BodyMesh 和 Mesh1P 上的两个网格体重新挂回武器自身的 RootComponent
	if (EquipmentMesh)
	{
		EquipmentMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	if (EquipmentMesh1P)
	{
		EquipmentMesh1P->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	
	if (HasAuthority())
	{
		// 重置旋转，确保掉落时始终朝上平躺
		FRotator CurrentRotation = GetActorRotation();
		CurrentRotation.Pitch = 0.f;
		CurrentRotation.Roll = 0.f;
		SetActorRotation(CurrentRotation);
		
		CollisionSphere->SetSimulatePhysics(true);
		CollisionSphere->SetEnableGravity(true);
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		
		// 丢弃武器时，给予一个向前的冲量（需在 SetOwner(nullptr) 前执行）
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
		GetWorldTimerManager().SetTimer(DestroyEquipmentTimerHandle, this, &ThisClass::DestroyEquipment, 15.f, false);
		
		SetOwner(nullptr);
		OwnerTeam = ETeam::NoTeam;
	}

	// 统一刷新为地面掉落的显隐和阴影状态
	UpdateViewMode();
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

	for (TSubclassOf<UGameplayAbility> AbilityClass : Abilities)
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