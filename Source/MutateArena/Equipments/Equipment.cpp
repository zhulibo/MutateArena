#include "Equipment.h"

#include "DataRegistrySubsystem.h"
#include "AnimInstance_Equipment.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Animation/AnimationAsset.h"
#include "MutateArena/MutateArena.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/TeamType.h"
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
	CollisionSphere->SetLinearDamping(1.f);
	CollisionSphere->SetSphereRadius(20.f);

	EquipmentMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EquipmentMesh"));
	EquipmentMesh->SetupAttachment(RootComponent);
	EquipmentMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EquipmentMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	EquipmentMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	EquipmentMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block); // 碰撞血液

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
}

void AEquipment::BeginPlay()
{
	Super::BeginPlay();

	SetReplicateMovement(true);

	FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(EquipmentName));
	FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
	if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
	{
		EquipmentParentName = EquipmentMain->EquipmentParentName;
		EquipmentCate = EquipmentMain->EquipmentCate;
		EquipmentType = EquipmentMain->EquipmentType;
	}
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

UAnimInstance_Equipment* AEquipment::GetEquipmentAnimInstance()
{
	if (EquipmentAnimInstance == nullptr) EquipmentAnimInstance = Cast<UAnimInstance_Equipment>(EquipmentMesh->GetAnimInstance());
	return EquipmentAnimInstance;
}

void AEquipment::MulticastHiddenMesh_Implementation()
{
	EquipmentMesh->SetVisibility(false);
}

void AEquipment::OnEquip()
{
	EquipmentState = EEquipmentState::Equipped;

	// 清除销毁定时器
	GetWorldTimerManager().ClearTimer(DestroyEquipmentTimerHandle);

	CollisionSphere->SetSimulatePhysics(false);
	CollisionSphere->SetEnableGravity(false);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetOwnerTeam();

	// 缓存BaseController
	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter) BaseController = Cast<ABaseController>(HumanCharacter->GetController());
}

void AEquipment::SetOwnerTeam()
{
	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter)
	{
		if (ABasePlayerState* PlayerState = Cast<ABasePlayerState>(HumanCharacter->GetPlayerState()))
		{
			OwnerTeam = PlayerState->Team;
		}
	}
}

void AEquipment::OnDrop()
{
	EquipmentState = EEquipmentState::Dropped;

	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetEnableGravity(true);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// HACK 延迟开启AreaSphere碰撞 确保武器已被丢出当前角色的Overlap区域
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::SetAreaSphereCollision, .4f);

	// 丢弃一定时间后销毁
	GetWorldTimerManager().SetTimer(DestroyEquipmentTimerHandle, this, &ThisClass::DestroyEquipment, 20.f);

	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	CollisionSphere->DetachFromComponent(DetachRules);
	// 丢弃武器时，给予一个向前的冲量
	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetOwner());
	if (HumanCharacter)
	{
		if (UCameraComponent* CameraComponent = HumanCharacter->FindComponentByClass<UCameraComponent>())
		{
			float Impulse = HumanCharacter->bIsDead ? 100.f : 300.f;
			CollisionSphere->AddImpulse(CameraComponent->GetForwardVector() * Impulse, NAME_None, true);
		}
	}

	SetOwner(nullptr);
	HumanCharacter = nullptr;
	BaseController = nullptr;
	OwnerTeam = ETeam::NoTeam;
}

void AEquipment::SetAreaSphereCollision()
{
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEquipment::DestroyEquipment()
{
	Destroy();
}
