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
#include "Data/CharacterCommonAsset.h"
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
#include "MutateArena/PlayerStates/BasePlayerState.h"

#define LOCTEXT_NAMESPACE "AHumanCharacter"

AHumanCharacter::AHumanCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	Tags.Add(Tags::CharacterHuman);

	BloodColor = Colors::Red;

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	RecoilComp = CreateDefaultSubobject<URecoilComponent>(TEXT("RecoilComponent"));
	CrosshairComp = CreateDefaultSubobject<UCrosshairComponent>(TEXT("CrosshairComponent"));

	DefaultMaxWalkSpeed = 500.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeed * 0.5f;
}

void AHumanCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsImmune);
}

void AHumanCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AHumanCharacter::BeginPlay()
{
	Super::BeginPlay();

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

void AHumanCharacter::OnASCInit()
{
	Super::OnASCInit();

	if (ASC && AttributeSetBase)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetMaxWalkSpeedAttribute()).AddUObject(this, &ThisClass::OnMaxWalkSpeedChanged);

		ASC->RegisterGameplayTagEvent(TAG_STATE_COMBAT_SWAPPING, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnSwappingTagChanged);
		ASC->RegisterGameplayTagEvent(TAG_STATE_COMBAT_AIMING, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnAimingTagChanged);
	}
}

void AHumanCharacter::OnMaxWalkSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
	GetCharacterMovement()->MaxWalkSpeedCrouched = Data.NewValue * 0.5f;
}

void AHumanCharacter::OnSwappingTagChanged(FGameplayTag GameplayTag, int NewCount)
{
	UpdateMaxWalkSpeed();

	if (IsLocallyControlled() && NewCount > 0 && CombatComp)
	{
		CombatComp->StartInterpFOV(CombatComp->DefaultFOV, 0.2f);
		CombatComp->AimingProgress = 0.f;
	}
}

void AHumanCharacter::OnAimingTagChanged(FGameplayTag GameplayTag, int NewCount)
{
	UpdateMaxWalkSpeed();

	if (CombatComp)
	{
		CombatComp->LocalSetAiming(NewCount > 0);
	}
}

void AHumanCharacter::UpdateMaxWalkSpeed()
{
	if (CombatComp && CombatComp->GetCurEquipment())
	{
		float TempMaxWalkSpeed = GetMaxWalkSpeed() * (CombatComp->IsAiming()
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

	// 本地Controller就绪后，在服务端生成装备，然后复制到所有客户端
	ServerSpawnEquipments(Slot1Name, Slot2Name, Slot3Name, Slot4Name);
}

void AHumanCharacter::ServerSpawnEquipments_Implementation(EEquipmentName Slot1Name, EEquipmentName Slot2Name, EEquipmentName Slot3Name, EEquipmentName Slot4Name)
{
	if (CombatComp == nullptr) return;
	
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

	// 封装装备生成辅助 Lambda
	auto SpawnEquipment = [this, World]<typename EquipActorType>(EEquipmentName EquipName) -> bool
	{

		if (EquipName == EEquipmentName::None) return false;

		UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(EquipName);
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
			SpawnedActor->EquipmentName = EquipName;
			SpawnedActor->FinishSpawning(FTransform::Identity);
			UE_LOG(LogTemp, Warning, TEXT("EquipName: %s"), *UEnum::GetDisplayValueAsText(EquipName).ToString());

			CombatComp->GiveEquipment(SpawnedActor);
			return true;
		}

		return false;
	};

	// 先装备副武器，以便被置为 LastEquipmentSlot
	SpawnEquipment.template operator()<AWeapon>(Slot2Name);

	// 主武器
	if (SpawnEquipment.template operator()<AWeapon>(Slot1Name))
	{
		CombatComp->InstantSwap(EEquipmentSlot::Slot1);
	}
	// 近战武器
	if (SpawnEquipment.template operator()<AMelee>(Slot3Name))
	{
		if (World->GetGameState<AMeleeGameState>() != nullptr)
		{
			CombatComp->InstantSwap(EEquipmentSlot::Slot3);
		}
	}
	// 投掷物
	SpawnEquipment.template operator()<AThrowable>(Slot4Name);
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

	if (!CombatComp) return;

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
	
	if (AWeapon* Slot1Weapon = Cast<AWeapon>(CombatComp->GetEquipmentByType(EEquipmentSlot::Slot1)))
	{
		EEquipmentSlot EquipmentSlot = EEquipmentSlot::Slot1;
		if (CombatComp->CurEquipmentSlot == EEquipmentSlot::Slot1 || CombatComp->CurEquipmentSlot != EEquipmentSlot::Slot1 && Slot1Weapon->IsEmpty())
		{
			EquipmentSlot = EEquipmentSlot::Slot2;
		}
		SendSwapEquipmentEvent(EquipmentSlot);
	}
}

void AHumanCharacter::SendSwapEquipmentEvent(EEquipmentSlot TargetEquipmentSlot)
{
	if (CombatComp && CombatComp->CurEquipmentSlot == TargetEquipmentSlot) return;

	if (ASC)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		Payload.EventMagnitude = static_cast<float>(TargetEquipmentSlot);
		ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_SWAP, &Payload);
	}
}

void AHumanCharacter::ClientSwapEquipmentWhenPickupFailed_Implementation(EEquipmentSlot FallbackEquipmentSlot)
{
	if (bIsDead || CombatComp == nullptr) return;
	if (CombatComp->CurEquipmentSlot == FallbackEquipmentSlot) return; // 如果本地客户端已经通过预测切到了目标武器，就不需要再切一次了，防止打断动画

	SendSwapEquipmentEvent(CombatComp->GetLastEquipment() ? CombatComp->LastEquipmentSlot : EEquipmentSlot::Slot3);
}

// 装备检测到的重叠装备
void AHumanCharacter::EquipOverlappingEquipment_Server(AEquipment* Equipment)
{
	if (bIsDead || Equipment == nullptr || Equipment->GetOwner() || CombatComp == nullptr) return;
	if (CombatComp->OwnEquipment(Equipment->EquipmentAsset->GetEquipmentSlot())) return;
	if (CombatComp->Slot3Equipment) // HACK 排除玩家刚重生还没有装备好武器（近战武器装备后无法丢弃）
	{
		CombatComp->GiveEquipment(Equipment);

		CombatComp->ClientPlayEquipSound();
	}
}

// 给予补给箱装备
void AHumanCharacter::ServerGivePickupEquipment_Implementation(APickupEquipment* PickupEquipment)
{
	if (bIsDead || CombatComp == nullptr || PickupEquipment == nullptr) return;

	AEquipment* Equipment = PickupEquipment->Equipment;
	if (Equipment == nullptr) return;

	if (Equipment->EquipmentAsset->GetEquipmentSlot() != EEquipmentSlot::Slot1 && Equipment->EquipmentAsset->GetEquipmentSlot() != EEquipmentSlot::Slot2)
	{
		UE_LOG(LogTemp, Error, TEXT("PickupEquipment EquipmentSlot is not Slot1 or Slot2"));
		return;
	}

	// 丢弃旧装备
	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
	FGameplayEventData Payload;
	Payload.EventMagnitude = static_cast<float>(Equipment->EquipmentAsset->GetEquipmentSlot());
	Payload.OptionalObject = PickupEquipment;
	ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_DROP, &Payload);
}

void AHumanCharacter::TrySwitchLoadout()
{
	if (CombatComp == nullptr) return;

	if (bCanSwitchLoadout)
	{
		CombatComp->ServerDestroyEquipments();

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
		// 直接在服务端丢弃主武器不走GA
		if (HasAuthority())
		{
			if (AEquipment* Slot1Equip = CombatComp->GetEquipmentByType(EEquipmentSlot::Slot1))
			{
				Slot1Equip->ClearAbilities(ASC);
				Slot1Equip->Drop();
			}
			
			CombatComp->ServerDestroyEquipments(false);
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
