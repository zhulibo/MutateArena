#include "HumanCharacter.h"

#include "CommonInputTypeEnum.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "TimerManager.h"
#include "MutateArena/Equipments/Throwable.h"
#include "Components/AutoHostComponent.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "Components/CombatComponent.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/System/Storage/ConfigType.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Pickups/PickupEquipment.h"
#include "MutateArena/GameModes/MeleeMode.h"
#include "MutateArena/GameModes/MutationMode.h"
#include "MutateArena/GameModes/TeamDeadMatchMode.h"
#include "MutateArena/GameStates/MeleeGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/CapsuleComponent.h"
#include "Components/CrosshairComponent.h"
#include "Components/RecoilComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/CharacterAsset.h"
#include "Data/CharacterType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/InputAsset.h"
#include "Engine/GameInstance.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/GameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "Animation/AnimInstance.h"
#include "GroomComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"

#define LOCTEXT_NAMESPACE "AHumanCharacter"

AHumanCharacter::AHumanCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	Tags.Add(ActorTags::HumanCharacter);

	BloodColor = Colors::Red;

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	RecoilComp = CreateDefaultSubobject<URecoilComponent>(TEXT("RecoilComponent"));
	CrosshairComp = CreateDefaultSubobject<UCrosshairComponent>(TEXT("CrosshairComponent"));
}

void AHumanCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsImmune);
}

void AHumanCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 组件是直接从 MetaHuman 中复制过来的，未在构造函数中创建，通过名字查找到对应组件
	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	GetComponents<USkeletalMeshComponent>(SkeletalMeshes);
	for (USkeletalMeshComponent* SkelMesh : SkeletalMeshes)
	{
		if (SkelMesh->GetName().Equals(TEXT("Body")))
		{
			BodyMesh = SkelMesh;
		}
		else if (SkelMesh->GetName().Equals(TEXT("Face")))
		{
			FaceMesh = SkelMesh;
		}
	}

	TArray<UGroomComponent*> AllGrooms;
	GetComponents<UGroomComponent>(AllGrooms);
	for (UGroomComponent* Groom : AllGrooms)
	{
		Groom->bCastHiddenShadow = true;
		Groom->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

		const FString GroomName = Groom->GetName();
		if (GroomName.Equals(TEXT("Beard")))
		{
			BeardComp = Groom;
		}
		else if (GroomName.Equals(TEXT("Eyebrows")))
		{
			EyebrowsComp = Groom;
		}
		else if (GroomName.Equals(TEXT("Eyelashes")))
		{
			EyelashesComp = Groom;
		}
		else if (GroomName.Equals(TEXT("Fuzz")))
		{
			FuzzComp = Groom;
		}
		else if (GroomName.Equals(TEXT("Hair")))
		{
			HairComp = Groom;
		}
		else if (GroomName.Equals(TEXT("Mustache")))
		{
			MustacheComp = Groom;
		}
	}
}

void AHumanCharacter::BeginPlay()
{
	check(BodyMesh); check(FaceMesh); check(BeardComp); check(EyebrowsComp); check(EyelashesComp); check(FuzzComp); check(HairComp); check(MustacheComp);

	// BodyMesh 的物理资产已包含头部，FaceMesh 不用设置碰撞
	FaceMesh->bCastHiddenShadow = true;

	Super::BeginPlay();
	
	if (!AssetSubsystem) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	check(AssetSubsystem);
    
	HumanAsset = AssetSubsystem->GetHumanAssetByName(HumanName);
	check(HumanAsset);

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HumanReceiveDamage);
	}
}

void AHumanCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr) return;
	
	UInputAsset* InputAsset = AssetSubsystem->InputAsset;
	if (!InputAsset) return;
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputAsset->HumanMappingContext, 200);
		}
	}

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(InputAsset->MoveAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(InputAsset->LookMouseAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(InputAsset->LookStickAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(InputAsset->FireAction, ETriggerEvent::Started, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);

		EIC->BindAction(InputAsset->AimAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
		EIC->BindAction(InputAsset->AimAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
		EIC->BindAction(InputAsset->FireAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
		EIC->BindAction(InputAsset->FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
		EIC->BindAction(InputAsset->ReloadAction, ETriggerEvent::Triggered, this, &ThisClass::ReloadButtonPressed);
		EIC->BindAction(InputAsset->DropAction, ETriggerEvent::Triggered, this, &ThisClass::DropButtonPressed);

		EIC->BindAction(InputAsset->SwapSlot1EquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentSlot::Slot1);
		EIC->BindAction(InputAsset->SwapSlot2EquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentSlot::Slot2);
		EIC->BindAction(InputAsset->SwapSlot3EquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentSlot::Slot3);
		EIC->BindAction(InputAsset->SwapSlot4EquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentSlot::Slot4);
		EIC->BindAction(InputAsset->SwapLastSlotEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapLastSlotEquipmentButtonPressed);
		EIC->BindAction(InputAsset->SwapBetweenSlot1Slot2EquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapBetweenSlot1Slot2EquipmentButtonPressed);
	}
}

void AHumanCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AHumanCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CharacterCommonAsset && ASC)
	{
		for (TSubclassOf<UGameplayAbility> AbilityClass : AssetSubsystem->CharacterCommonAsset->HumanDefaultAbilities)
		{
			FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
			ASC->GiveAbility(AbilitySpec);
		}
	}
}

void AHumanCharacter::SetFirstPersonViewMode(bool bFirstPerson)
{
	Super::SetFirstPersonViewMode(bFirstPerson);

	// 切换 MetaHuman 面部与毛发组件显隐（BodyMesh 已由基类通过 GetMesh3P() 处理）
	FaceMesh->SetVisibility(!bFirstPerson);
	BeardComp->SetVisibility(!bFirstPerson);
	EyebrowsComp->SetVisibility(!bFirstPerson);
	EyelashesComp->SetVisibility(!bFirstPerson);
	FuzzComp->SetVisibility(!bFirstPerson);
	HairComp->SetVisibility(!bFirstPerson);
	MustacheComp->SetVisibility(!bFirstPerson);

	// 自动刷新当前角色身上挂载的所有武器（AEquipment）的 1P/3P 显隐与阴影状态
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* Actor : AttachedActors)
	{
		if (AEquipment* Equipment = Cast<AEquipment>(Actor))
		{
			Equipment->UpdateViewMode();
		}
	}
}

void AHumanCharacter::InitAbilityActorInfo()
{
	Super::InitAbilityActorInfo();

	// GAS 的 InitAbilityActorInfo 默认会绑定 ACharacter::GetMesh()，而人类第三人称 MetaHuman 身体网格体和动画蓝图挂在子组件 BodyMesh 上，
	// 因此需要手动将 AbilityActorInfo 的骨骼网格体与动画实例重定向到 BodyMesh，确保 PlayMontageAndWait 等任务能找到正确的 AnimInstance。
	if (ASC && BodyMesh && ASC->AbilityActorInfo.IsValid())
	{
		ASC->AbilityActorInfo->SkeletalMeshComponent = BodyMesh;
		ASC->AbilityActorInfo->AnimInstance = BodyMesh->GetAnimInstance();
	}
}

void AHumanCharacter::OnASCInit()
{
	Super::OnASCInit();

	if (ASC && AttributeSetBase)
	{
		ASC->RegisterGameplayTagEvent(TAG_STATE_COMBAT_SWAPPING, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnSwappingTagChanged);
		// 瞄准移速与镜头 FOV 已由 UGA_Equipment_Aim 在 ActivateAbility/EndAbility 中直接驱动，
		// 不监听 TAG_STATE_COMBAT_AIMING，防止服务端延迟同步的 MinimalReplicatedTags 覆盖客户端本地取消瞄准状态。
	}
}

void AHumanCharacter::OnSwappingTagChanged(FGameplayTag GameplayTag, int NewCount)
{
	UpdateMaxWalkSpeed();
}

void AHumanCharacter::UpdateMaxWalkSpeed()
{
	if (CombatComp && CombatComp->GetCurEquipment())
	{
		float TempMaxWalkSpeed = GetMaxWalkSpeed() * (CombatComp->bIsAiming
			                                              ? CombatComp->GetCurEquipment()->EquipmentAsset->GetAimingWalkSpeedMul()
			                                              : CombatComp->GetCurEquipment()->EquipmentAsset->GetWalkSpeedMul());
		GetCharacterMovement()->MaxWalkSpeed = TempMaxWalkSpeed;
		GetCharacterMovement()->MaxWalkSpeedCrouched = TempMaxWalkSpeed * 0.5f;
	}
}

void AHumanCharacter::OnLocallyControllerReady()
{
	Super::OnLocallyControllerReady();

	ApplyLoadout();
}

void AHumanCharacter::ApplyLoadout()
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem == nullptr) return;
	
	EEquipmentName Slot1Name = EEquipmentName::None;
	EEquipmentName Slot2Name = EEquipmentName::None;
	EEquipmentName Slot3Name = EEquipmentName::None;
	EEquipmentName Slot4Name = EEquipmentName::None;

	if (StorageSubsystem->CacheLoadout && StorageSubsystem->CacheSetting)
	{
		int32 LoadoutIndex = 0;
		const int32 TempLoadoutIndex = StorageSubsystem->CacheLoadout->LoadoutIndex;
		if (StorageSubsystem->CacheLoadout->Loadouts.IsValidIndex(TempLoadoutIndex))
		{
			LoadoutIndex = TempLoadoutIndex;
		}

		if (StorageSubsystem->CacheLoadout->Loadouts.IsValidIndex(LoadoutIndex))
		{
			const auto& SelectedLoadout = StorageSubsystem->CacheLoadout->Loadouts[LoadoutIndex];
			Slot1Name = SelectedLoadout.Slot1EquipmentName;
			Slot2Name = SelectedLoadout.Slot2EquipmentName;
			Slot3Name = SelectedLoadout.Slot3EquipmentName;
			Slot4Name = SelectedLoadout.Slot4EquipmentName;
		}
	}

	ServerSpawnEquipments(Slot1Name, Slot2Name, Slot3Name, Slot4Name);
}

void AHumanCharacter::ServerSpawnEquipments_Implementation(EEquipmentName Slot1Name, EEquipmentName Slot2Name, EEquipmentName Slot3Name, EEquipmentName Slot4Name)
{
	if (bIsDead || !bCanSwitchLoadout || CombatComp == nullptr) return;

	// 若角色身上已有旧装备（如开局阶段切换 Loadout），先在服务端统一销毁清理
	CombatComp->ServerDestroyEquipments_Implementation(true);
	
#if WITH_EDITOR
	if (const UDevSetting* DevSetting = GetDefault<UDevSetting>())
	{
		if (DevSetting->bIsUseCustomEquipment)
		{
			Slot1Name = DevSetting->Slot1EquipmentName;
			Slot2Name = DevSetting->Slot2EquipmentName;
			Slot3Name = DevSetting->Slot3EquipmentName;
			Slot4Name = DevSetting->Slot4EquipmentName;
		}
	}
#endif

	UWorld* World = GetWorld();
	if (!World) return;

	if (!AssetSubsystem) AssetSubsystem = World->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (!AssetSubsystem) return;

	// 封装装备生成辅助 Lambda（仅负责生成并入库收起）
	auto SpawnEquipment = [this, World]<typename EquipActorType>(EEquipmentName EquipmentName) -> bool
	{
		if (EquipmentName == EEquipmentName::None) return false;

		UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(EquipmentName);
		if (!Asset) return false;
		
		TSubclassOf<AEquipment> EquipClass = UDataAssetManager::Get().GetSubclass(Asset->GetEquipmentClass());
		if (!EquipClass || !EquipClass->IsChildOf<EquipActorType>()) return false;

		EquipActorType* SpawnedActor = World->SpawnActorDeferred<EquipActorType>(
			EquipClass,
			FTransform::Identity,
			this,
			this,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (SpawnedActor)
		{
			SpawnedActor->EquipmentName = EquipmentName;
			SpawnedActor->FinishSpawning(FTransform::Identity);
			UE_LOG(LogTemp, Warning, TEXT("EquipmentName: %s"), *UEnum::GetDisplayValueAsText(EquipmentName).ToString());

			CombatComp->GiveEquipment(SpawnedActor);
			return true;
		}

		return false;
	};

	// 1. 所有武器统一入库（此时 CurEquipmentSlot 为 None，全部自动进入已入库隐藏状态）
	const bool bHasSlot1 = SpawnEquipment.template operator()<AWeapon>(Slot1Name);
	const bool bHasSlot2 = SpawnEquipment.template operator()<AWeapon>(Slot2Name);
	const bool bHasSlot3 = SpawnEquipment.template operator()<AMelee>(Slot3Name);
	SpawnEquipment.template operator()<AThrowable>(Slot4Name);

	// 2. 决定初始激活槽位
	const bool bIsMeleeMode = World->GetGameState<AMeleeGameState>() != nullptr;
	EEquipmentSlot InitialActiveSlot = EEquipmentSlot::None;

	if (bIsMeleeMode && bHasSlot3)
	{
		InitialActiveSlot = EEquipmentSlot::Slot3;
	}
	else if (bHasSlot1)
	{
		InitialActiveSlot = EEquipmentSlot::Slot1;
	}
	else if (bHasSlot2)
	{
		InitialActiveSlot = EEquipmentSlot::Slot2;
	}
	else if (bHasSlot3)
	{
		InitialActiveSlot = EEquipmentSlot::Slot3;
	}

	// 3. 显式计算并写入 LastEquipmentSlot（优先副武器，其次近战武器）
	EEquipmentSlot InitialLastSlot = EEquipmentSlot::None;
	if (bHasSlot2 && InitialActiveSlot != EEquipmentSlot::Slot2)
	{
		InitialLastSlot = EEquipmentSlot::Slot2;
	}
	else if (bHasSlot3 && InitialActiveSlot != EEquipmentSlot::Slot3)
	{
		InitialLastSlot = EEquipmentSlot::Slot3;
	}

	// 4. 单独激活目标武器并写入 LastEquipmentSlot
	if (InitialActiveSlot != EEquipmentSlot::None)
	{
		CombatComp->InstantSwap(InitialActiveSlot);
	}
	CombatComp->LastEquipmentSlot = InitialLastSlot;
}

void AHumanCharacter::AimButtonPressed(const FInputActionValue& Value)
{
	if (CombatComp == nullptr || !ASC) return;

	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());

	if (CombatComp->GetCurWeapon())
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_EQUIPMENT_AIM));
	}
	else if (CombatComp->CurEquipmentSlot == EEquipmentSlot::Slot3)
	{
		// 需传参用 HandleGameplayEvent， GA 中需配置 AbilityTriggers Tag
		FGameplayEventData Payload;
		Payload.EventMagnitude = 2.f; // 重击
		if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
		if (BasePlayerState && BasePlayerState->InputType == ECommonInputType::Gamepad)
		{
			Payload.EventMagnitude = 1.f; // 手柄瞄准键绑的是轻击
		}
		ASC->HandleGameplayEvent(TAG_EVENT_MELEE_ATTACK, &Payload);
	}
}

void AHumanCharacter::AimButtonReleased(const FInputActionValue& Value)
{
#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bIsAdjustEquipmentSocketTransform) return;
	}
#endif

	if (!CombatComp || !ASC) return;

	if (CombatComp->GetCurWeapon()) // 目前限制只有主副武器可瞄准
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(TAG_ABILITY_EQUIPMENT_AIM);
		ASC->CancelAbilities(&CancelTags);
	}

	if (CombatComp->GetCurMelee())
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		ASC->HandleGameplayEvent(TAG_EVENT_MELEE_RELEASED, &Payload);
	}
}

void AHumanCharacter::FireButtonPressed(const FInputActionValue& Value)
{
	if (!ASC) return;

	bCanSwitchLoadout = false;

	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());

	switch (CombatComp->CurEquipmentSlot)
	{
	case EEquipmentSlot::Slot1:
	case EEquipmentSlot::Slot2:
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_WEAPON_FIRE));
		break;
	case EEquipmentSlot::Slot3:
		{
			// 需传参用 HandleGameplayEvent， GA 中需配置 AbilityTriggers Tag
			FGameplayEventData Payload;
			Payload.EventMagnitude = 1.f; // 轻击
			if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
			if (BasePlayerState && BasePlayerState->InputType == ECommonInputType::Gamepad)
			{
				Payload.EventMagnitude = 2.f; // 手柄开火键绑的是轻击
			}
			ASC->HandleGameplayEvent(TAG_EVENT_MELEE_ATTACK, &Payload);
		}
		break;
	case EEquipmentSlot::Slot4:
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_THROWABLE_THROW));
		break;
	}
}

void AHumanCharacter::FireButtonReleased(const FInputActionValue& Value)
{
	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());

	switch (CombatComp->CurEquipmentSlot)
	{
	case EEquipmentSlot::Slot1:
	case EEquipmentSlot::Slot2:
		{
			FGameplayTagContainer AbilityTags;
			AbilityTags.AddTag(TAG_ABILITY_WEAPON_FIRE);
			ASC->CancelAbilities(&AbilityTags);
		}
		break;
	case EEquipmentSlot::Slot3:
		{
			FGameplayEventData Payload;
			ASC->HandleGameplayEvent(TAG_EVENT_MELEE_RELEASED, &Payload);
		}
		break;
	}
}

void AHumanCharacter::ReloadButtonPressed(const FInputActionValue& Value)
{
	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
	ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_WEAPON_RELOAD));
}

void AHumanCharacter::DropButtonPressed(const FInputActionValue& Value)
{
	if (!CombatComp || !CombatComp->GetCurWeapon()) return; // 只有主副武器可以丢弃

	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
	FGameplayEventData Payload;
	Payload.EventMagnitude = static_cast<float>(CombatComp->CurEquipmentSlot);
	ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_DROP, &Payload);

	bCanSwitchLoadout = false;
}

void AHumanCharacter::SwapLastSlotEquipmentButtonPressed(const FInputActionValue& Value)
{
	if (!CombatComp) return;
	
	SendSwapEquipmentEvent(CombatComp->LastEquipmentSlot);
}

void AHumanCharacter::SwapBetweenSlot1Slot2EquipmentButtonPressed(const FInputActionValue& Value)
{
	if (!CombatComp) return;

	AWeapon* Slot1Weapon = Cast<AWeapon>(CombatComp->GetEquipmentBySlot(EEquipmentSlot::Slot1));
	EEquipmentSlot TargetSlot = EEquipmentSlot::Slot1;

	if (CombatComp->CurEquipmentSlot == EEquipmentSlot::Slot1 || !Slot1Weapon || Slot1Weapon->IsEmpty())
	{
		TargetSlot = EEquipmentSlot::Slot2;
	}

	SendSwapEquipmentEvent(TargetSlot);
}

void AHumanCharacter::SendSwapEquipmentEvent(EEquipmentSlot TargetEquipmentSlot)
{
	if (!CombatComp) return;
	if (TargetEquipmentSlot == EEquipmentSlot::None || CombatComp->CurEquipmentSlot == TargetEquipmentSlot) return;
	if (!CombatComp->HasEquipmentInSlot(TargetEquipmentSlot)) return;

	if (ASC)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		Payload.EventMagnitude = static_cast<float>(TargetEquipmentSlot);
		ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_SWAP, &Payload);
	}
}

// 给予检测到的重叠装备
void AHumanCharacter::GiveOverlappingEquipment(AEquipment* Equipment)
{
	if (bIsDead || Equipment == nullptr || Equipment->GetOwner() || CombatComp == nullptr) return;
	if (CombatComp->HasEquipmentInSlot(Equipment->EquipmentAsset->GetEquipmentSlot())) return;
	
	bCanSwitchLoadout = false;
	CombatComp->GiveEquipment(Equipment);
	CombatComp->ClientPlayEquipSound();
}

// 给予补给箱装备（服务端权威调用）
void AHumanCharacter::GivePickupEquipment(APickupEquipment* PickupEquipment)
{
	if (!HasAuthority() || bIsDead || CombatComp == nullptr || !IsValid(PickupEquipment) || !ASC) return;

	AEquipment* NewEquipment = PickupEquipment->Equipment;
	if (NewEquipment == nullptr) return;

	const UEquipmentAsset* EquipAsset = NewEquipment->EnsureEquipmentAsset();
	if (EquipAsset == nullptr) return;

	const EEquipmentSlot TargetSlot = EquipAsset->GetEquipmentSlot();
	if (TargetSlot != EEquipmentSlot::Slot1 && TargetSlot != EEquipmentSlot::Slot2)
	{
		UE_LOG(LogTemp, Error, TEXT("PickupEquipment EquipmentSlot is not Slot1 or Slot2"));
		return;
	}

	bCanSwitchLoadout = false;

	// 立即置空补给箱内的武器指针并销毁补给箱，防止同帧多次触发
	PickupEquipment->Equipment = nullptr;
	PickupEquipment->Destroy();

	const bool bIsReplacingCurrent = (CombatComp->CurEquipmentSlot == TargetSlot);

	// 1. 若目标槽位已有旧武器，在服务端直接执行丢弃与清理
	if (AEquipment* OldEquipment = CombatComp->GetEquipmentBySlot(TargetSlot))
	{
		OldEquipment->OnStartSwapOut();
		OldEquipment->ClearAbilities(ASC);
		OldEquipment->Drop();

		FGameplayCueParameters CueParams;
		CueParams.Instigator = this;
		CueParams.SourceObject = OldEquipment;
		ASC->ExecuteGameplayCue(TAG_GC_EQUIPMENT_DROP, CueParams);
	}

	// 2. 若替换的是当前手持槽位，打断当前武器动作，并临时将 CurEquipmentSlot 置为 None
	// 这样后续 GiveEquipment 会先将其作为 Holstered 入库，紧接着触发 GA_Equipment_Swap 即可走通完整的切枪状态与动画！
	if (bIsReplacingCurrent)
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(TAG_ABILITY_WEAPON_FIRE);
		CancelTags.AddTag(TAG_ABILITY_EQUIPMENT_AIM);
		CancelTags.AddTag(TAG_ABILITY_WEAPON_RELOAD);
		ASC->CancelAbilities(&CancelTags);

		// 若为监听服务器房主（Host），直接停止本地旧 1P 动画
		if (IsLocallyControlled())
		{
			if (UAnimInstance* AnimInst1P = Mesh1P ? Mesh1P->GetAnimInstance() : nullptr)
			{
				AnimInst1P->Montage_Stop(0.f);
			}
		}

		CombatComp->CurEquipmentSlot = EEquipmentSlot::None;
	}

	// 3. 给予新装备入库
	CombatComp->GiveEquipment(NewEquipment);

	// 4. 若替换的是当前手持槽位，触发切枪 GA；否则仅通知客户端播放拾取音效
	if (bIsReplacingCurrent)
	{
		FGameplayEventData SwapPayload;
		SwapPayload.EventMagnitude = static_cast<float>(TargetSlot);
		ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_SWAP, &SwapPayload);
	}
	else
	{
		CombatComp->ClientPlayEquipSound();
	}
}

void AHumanCharacter::TrySwitchLoadout()
{
	if (CombatComp == nullptr) return;

	if (bCanSwitchLoadout)
	{
		ApplyLoadout();
	}
	else
	{
		NOTIFY(this, Colors::White, LOCTEXT("LoadoutApplyTip", "Loadout will apply in the next respawn"));
	}
}

void AHumanCharacter::HumanReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                         AController* AttackerController, AActor* DamageCauser)
{
	if (bIsDead) return;

	if (BaseMode == nullptr) BaseMode = GetWorld()->GetAuthGameMode<ABaseMode>();
	if (BaseMode)
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
		if (BaseController)
		{
			BaseMode->HumanReceiveDamage(this, BaseController, FMath::RoundToInt(Damage), DamageType, AttackerController, DamageCauser);
		}
	}
}

void AHumanCharacter::OnRep_bIsDead()
{
	Super::OnRep_bIsDead();

	if (bIsDead)
	{
		OnDead();
	}
}

void AHumanCharacter::MutationDead(bool bNeedSpawn, ESpawnMutantReason SpawnMutantReason)
{
	bIsDead = true;
	OnDead();

	if (HasAuthority() && bNeedSpawn)
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this, SpawnMutantReason]()
		{
			if (AMutationMode* MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>())
			{
				MutationMode->Mutate(this, Controller, SpawnMutantReason);
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AHumanCharacter::MeleeDead()
{
	bIsDead = true;
	OnDead();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]()
		{
			if (AMeleeMode* MeleeMode = GetWorld()->GetAuthGameMode<AMeleeMode>())
			{
				MeleeMode->HumanRespawn(this, Controller);
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AHumanCharacter::TeamDeadMatchDead()
{
	bIsDead = true;
	OnDead();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]()
		{
			if (ATeamDeadMatchMode* TeamDeadMatchMode = GetWorld()->GetAuthGameMode<ATeamDeadMatchMode>())
			{
				TeamDeadMatchMode->HumanRespawn(this, Controller);
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AHumanCharacter::OnDead()
{
	Super::OnDead();

	if (CombatComp)
	{
		// 所有端立即收起隐藏非主武器，防止客户端在等待销毁网络包到达前调用 SetFirstPersonViewMode(false) 导致手枪/刀在尸体手上闪现穿帮
		for (EEquipmentSlot Slot : { EEquipmentSlot::Slot2, EEquipmentSlot::Slot3, EEquipmentSlot::Slot4 })
		{
			if (AEquipment* Equip = CombatComp->GetEquipmentBySlot(Slot))
			{
				Equip->SetState(EEquipmentState::Holstered);
			}
		}
		
		// 直接在服务端丢弃主武器不走GA
		if (HasAuthority())
		{
			if (AEquipment* Slot1Equip = CombatComp->GetEquipmentBySlot(EEquipmentSlot::Slot1))
			{
				Slot1Equip->ClearAbilities(ASC);
				Slot1Equip->Drop();
				CombatComp->Slot1Equipment = nullptr; // 置空主武器槽位指针，断开与地面掉落武器的关联
			}
			
			CombatComp->ServerDestroyEquipments_Implementation(false);
		}
	}

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController) DisableInput(BaseController);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetSimulatePhysics(true);
	BodyMesh->SetEnableGravity(true);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	SetFirstPersonViewMode(false);
}

void AHumanCharacter::BecomeImmune()
{
	bIsImmune = true;
	OnRep_bIsImmune();
}

void AHumanCharacter::OnRep_bIsImmune()
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController && IsLocallyControlled())
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			if (bIsImmune)
			{
				UISubsystem->OnBeImmune.Broadcast();
			}
		}
	}

	if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(LocalPC->GetLocalPlayer()))
		{
			UISubsystem->OnOverheadWidgetNeedUpdate.Broadcast();
		}
	}
}

#undef LOCTEXT_NAMESPACE
