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
	DOREPLIFETIME(ThisClass, DropInfo);
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

	// 客户端收到 Owner 同步时刷新一次视角状态，避免 Owner 与状态网络包先后到达引起的显隐偏差
	UpdateViewMode();
}

void AEquipment::OnAreaSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (AHumanCharacter* OverlapHumanCharacter = Cast<AHumanCharacter>(OtherActor))
	{
		// 丢弃者短暂不能立刻捡回刚扔出的装备；其他玩家不受影响。
		if (LastDropper.Get() == OverlapHumanCharacter && GetWorld()->GetTimeSeconds() < LastDropperPickupBlockedUntil)
		{
			return;
		}

		OverlapHumanCharacter->GiveOverlappingEquipment(this);
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

USkeletalMeshComponent* AEquipment::GetVisibleEquipmentMesh() const
{
	if (const AHumanCharacter* OwnerCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		if (OwnerCharacter->bIsCurrentViewTarget && EquipmentMesh1P)
		{
			return EquipmentMesh1P;
		}
	}

	return EquipmentMesh;
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

		// 清除销毁定时器
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
	if (!HasAuthority() || EquipmentState == EEquipmentState::Dropped)
	{
		return;
	}

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	LastDropper = HumanCharacter;
	LastDropperPickupBlockedUntil = GetWorld()->GetTimeSeconds() + 0.4;

	FVector ThrowDir = GetActorForwardVector();
	FVector DropStartLoc = GetActorLocation();
	FRotator DropRotation = GetActorRotation();
	FVector InitialVelocity = FVector::ZeroVector;

	if (HumanCharacter)
	{
		const FRotator AimRot = HumanCharacter->GetBaseAimRotation();
		ThrowDir = AimRot.Vector();

		FVector EyeLoc;
		FRotator EyeRot;
		HumanCharacter->GetActorEyesViewPoint(EyeLoc, EyeRot);

		DropStartLoc = EyeLoc + ThrowDir * 30.f - FVector(0.f, 0.f, 15.f);
		DropRotation = FRotator(0.f, AimRot.Yaw, 0.f);

		const float Impulse = HumanCharacter->bIsDead ? 100.f : 400.f;
		InitialVelocity = ThrowDir * Impulse;
	}
	else
	{
		DropRotation.Pitch = 0.f;
		DropRotation.Roll = 0.f;
	}

	// 开启移动复制
	SetReplicateMovement(true);

	EquipmentState = EEquipmentState::Dropped;

	// 保存服务器计算的最终掉落信息
	DropInfo.Transform = FTransform(
		DropRotation,
		DropStartLoc,
		GetActorScale3D()
	);

	DropInfo.LinearVelocity = InitialVelocity;
	++DropInfo.Sequence;

	// 服务器应用掉落信息
	HandleDrop();

	// 清除 Owner
	SetOwner(nullptr);
	OwnerTeam = ETeam::NoTeam;

	// 服务器负责销毁
	GetWorldTimerManager().SetTimer(
		DestroyEquipmentTimerHandle,
		this,
		&ThisClass::DestroyEquipment,
		200.f,
		false
	);

	ForceNetUpdate();
}

void AEquipment::OnRep_DropInfo()
{
	// 确保 EquipmentState 已经进入掉落状态
	EquipmentState = EEquipmentState::Dropped;

	// 使用服务器复制的 Transform 和初始速度
	HandleDrop();
}

void AEquipment::HandleDrop()
{
	// 先分离 Actor 的 RootComponent，保持世界变换
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 将 1P 和 3P 网格体重新挂回武器 RootComponent
	if (EquipmentMesh)
	{
		EquipmentMesh->AttachToComponent(
			RootComponent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
	}

	if (EquipmentMesh1P)
	{
		EquipmentMesh1P->AttachToComponent(
			RootComponent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
	}

	// 使用服务器复制的世界 Transform
	SetActorTransform(
		DropInfo.Transform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	// 开启物理
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetEnableGravity(true);
	CollisionSphere->SetSimulatePhysics(true);

	// 使用服务器提供的初始速度
	CollisionSphere->SetPhysicsLinearVelocity(DropInfo.LinearVelocity);
	CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	CollisionSphere->WakeAllRigidBodies();

	// 立即允许其他玩家拾取；丢弃者由 OnAreaSphereOverlap 单独短暂忽略。
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	UpdateViewMode();
}

void AEquipment::SetState(EEquipmentState NewState)
{
	EquipmentState = NewState;
	UpdateViewMode();
}

void AEquipment::OnRep_EquipmentState(EEquipmentState OldState)
{
	UpdateViewMode();

	// Dropped 状态由 OnRep_DropInfo 统一处理
}

void AEquipment::UpdateViewMode()
{
	// 1. 收起在背包中的武器：1P 和 3P 模型均不显示，且关闭隐藏阴影避免地面多出悬空枪影
	if (EquipmentState == EEquipmentState::Holstered)
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

	// 3. 武器当前装备在角色手上（Equipped）：根据持有者是否为当前客户端的 ViewTarget 切换 1P/3P
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

void AEquipment::DestroyEquipment()
{
	Destroy();
}

void AEquipment::GiveAbilities(UAbilitySystemComponent* ASC)
{
	if (!ASC || !HasAuthority()) return;

	const UEquipmentAsset* Asset = EnsureEquipmentAsset();
	if (!Asset) return;

	for (const TSoftClassPtr<UGameplayAbility>& AbilitySoftClass : Asset->GetGrantedAbilities())
	{
		if (TSubclassOf<UGameplayAbility> AbilityClass = UDataAssetManager::Get().GetSubclass(AbilitySoftClass))
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
