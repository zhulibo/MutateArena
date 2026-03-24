#include "HumanCharacter.h"

#include "CommonInputTypeEnum.h"
#include "DataRegistryId.h"
#include "DataRegistrySubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "..\Equipments\Throwing.h"
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
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/CapsuleComponent.h"
#include "Components/CrosshairComponent.h"
#include "Components/RecoilComponent.h"
#include "Data/CharacterAsset.h"
#include "Data/CharacterType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/InputAsset.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"

#define LOCTEXT_NAMESPACE "AHumanCharacter"

AHumanCharacter::AHumanCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	RecoilComp = CreateDefaultSubobject<URecoilComponent>(TEXT("RecoilComponent"));
	CrosshairComp = CreateDefaultSubobject<UCrosshairComponent>(TEXT("CrosshairComponent"));
	
	BloodColor = C_RED;
	
	DefaultMaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeed * 0.5f;
	
	Tags.Add(TAG_CHARACTER_HUMAN);
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
	if (AssetSubsystem == nullptr || AssetSubsystem->InputAsset == nullptr) return;

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(AssetSubsystem->InputAsset->HumanMappingContext, 200);
		}
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(AssetSubsystem->InputAsset->LookMouseAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(AssetSubsystem->InputAsset->LookStickAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(AssetSubsystem->InputAsset->FireAction, ETriggerEvent::Started, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		
		EIC->BindAction(AssetSubsystem->InputAsset->AimAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->AimAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
		EIC->BindAction(AssetSubsystem->InputAsset->FireAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
		EIC->BindAction(AssetSubsystem->InputAsset->ReloadAction, ETriggerEvent::Triggered, this, &ThisClass::ReloadButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->DropAction, ETriggerEvent::Triggered, this, &ThisClass::DropButtonPressed);
		
		EIC->BindAction(AssetSubsystem->InputAsset->SwapPrimaryEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentType::Primary);
		EIC->BindAction(AssetSubsystem->InputAsset->SwapSecondaryEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentType::Secondary);
		EIC->BindAction(AssetSubsystem->InputAsset->SwapMeleeEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentType::Melee);
		EIC->BindAction(AssetSubsystem->InputAsset->SwapThrowingEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SendSwapEquipmentEvent, EEquipmentType::Throwing);
		EIC->BindAction(AssetSubsystem->InputAsset->SwapLastEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapLastEquipmentButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->SwapBetweenPrimarySecondaryEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapBetweenPrimarySecondaryEquipmentButtonPressed);
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
	if (AssetSubsystem && AssetSubsystem->CharacterAsset && ASC)
	{
		for (TSubclassOf<UGameplayAbility> AbilityClass : AssetSubsystem->CharacterAsset->HumanDefaultAbilities)
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

		ASC->RegisterGameplayTagEvent(TAG_STATE_COMBAT_SWAPPING, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AHumanCharacter::OnSwappingTagChanged);
		ASC->RegisterGameplayTagEvent(TAG_STATE_COMBAT_AIMING, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AHumanCharacter::OnAimingTagChanged);
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
		float TempMaxWalkSpeed = GetMaxWalkSpeed() * (CombatComp->IsAiming() ? CombatComp->GetCurEquipment()->AimingWalkSpeedMul : CombatComp->GetCurEquipment()->WalkSpeedMul);
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
	// 获取在用的Loadout
	int32 LoadoutIndex = 0;
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem && StorageSubsystem->CacheSetting)
	{
		int32 TempLoadoutIndex = StorageSubsystem->CacheLoadout->LoadoutIndex;
		if (TempLoadoutIndex > 0 && TempLoadoutIndex < StorageSubsystem->CacheLoadout->Loadouts.Num())
		{
			LoadoutIndex = TempLoadoutIndex;
		}
	}

	// 本地Controller就绪后，在服务端生成装备，然后复制到所有客户端
	ServerSpawnEquipments(
		GetEquipmentName(LoadoutIndex, EEquipmentType::Primary),
		GetEquipmentName(LoadoutIndex, EEquipmentType::Secondary),
		GetEquipmentName(LoadoutIndex, EEquipmentType::Melee),
		GetEquipmentName(LoadoutIndex, EEquipmentType::Throwing)
	);
}

void AHumanCharacter::ServerSpawnEquipments_Implementation(EEquipmentName Primary, EEquipmentName Secondary, EEquipmentName Melee, EEquipmentName Throwing)
{
	if (CombatComp == nullptr) return;

	TSubclassOf<AEquipment> PrimaryClass = nullptr;
	TSubclassOf<AEquipment> SecondaryClass = nullptr;
	TSubclassOf<AEquipment> MeleeClass = nullptr;
	TSubclassOf<AEquipment> ThrowingClass = nullptr;

	FName PrimaryName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Primary)));
	FName SecondaryName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Secondary)));
	FName MeleeName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Melee)));
	FName ThrowingName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Throwing)));

	// 近战模式只生成近战装备
	if (AMeleeGameState* MeleeGameState = GetWorld()->GetGameState<AMeleeGameState>())
	{
		PrimaryName = FName();
		SecondaryName = FName();
		ThrowingName = FName();
	}

	// 包括Standalone
#if UE_EDITOR
	if (GetDefault<UDevSetting>()->bIsUseCustomEquipment)
	{
		PrimaryName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(GetDefault<UDevSetting>()->PrimaryEquipment)));
		SecondaryName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(GetDefault<UDevSetting>()->SecondaryEquipment)));
		MeleeName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(GetDefault<UDevSetting>()->MeleeEquipment)));
		ThrowingName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(GetDefault<UDevSetting>()->ThrowingEquipment)));
	}
#endif

	if (PrimaryName.IsValid()) {
		FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, PrimaryName);
		if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
		{
			PrimaryClass = UDataAssetManager::Get().GetSubclass(EquipmentMain->EquipmentClass);
		}
	}
	if (SecondaryName.IsValid()) {
		FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, SecondaryName);
		if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
		{
			SecondaryClass = UDataAssetManager::Get().GetSubclass(EquipmentMain->EquipmentClass);
		}
	}
	if (MeleeName.IsValid()) {
		FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, MeleeName);
		if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
		{
			MeleeClass = UDataAssetManager::Get().GetSubclass(EquipmentMain->EquipmentClass);
		}
	}
	if (ThrowingName.IsValid()) {
		FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, ThrowingName);
		if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
		{
			ThrowingClass = UDataAssetManager::Get().GetSubclass(EquipmentMain->EquipmentClass);
		}
	}
	
	// 先装备副武器，以便被置为LastEquipmentType
	if (SecondaryClass && SecondaryClass->IsChildOf<AWeapon>())
	{
		AWeapon* SpawnedActor = GetWorld()->SpawnActor<AWeapon>(SecondaryClass);
		CombatComp->SecondaryEquipment = SpawnedActor;
		CombatComp->EquipEquipment(SpawnedActor);
	}
	if (PrimaryClass && PrimaryClass->IsChildOf<AWeapon>())
	{
		AWeapon* SpawnedActor = GetWorld()->SpawnActor<AWeapon>(PrimaryClass);
		CombatComp->PrimaryEquipment = SpawnedActor;
		CombatComp->EquipEquipment(SpawnedActor);
		
		CombatComp->InstantSwap(EEquipmentType::Primary);
	}
	if (MeleeClass && MeleeClass->IsChildOf<AMelee>())
	{
		AMelee* SpawnedActor = GetWorld()->SpawnActor<AMelee>(MeleeClass);
		CombatComp->MeleeEquipment = SpawnedActor;
		CombatComp->EquipEquipment(SpawnedActor);
		
		// 近战模式默认切到近战武器
		if (AMeleeGameState* MeleeGameState = GetWorld()->GetGameState<AMeleeGameState>())
		{
			CombatComp->InstantSwap(EEquipmentType::Melee);
		}
	}
	if (ThrowingClass && ThrowingClass->IsChildOf<AThrowing>())
	{
		AThrowing* SpawnedActor = GetWorld()->SpawnActor<AThrowing>(ThrowingClass);
		CombatComp->ThrowingEquipment = SpawnedActor;
		CombatComp->EquipEquipment(SpawnedActor);
	}
}

// 获取装备的名字
EEquipmentName AHumanCharacter::GetEquipmentName(int32 LoadoutIndex, EEquipmentType EquipmentType)
{
	EEquipmentName EquipmentName = EEquipmentName::None;

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem == nullptr || StorageSubsystem->CacheLoadout->Loadouts.Num() < LoadoutIndex + 1)
	{
		return EquipmentName;
	}

	switch (EquipmentType)
	{
	case EEquipmentType::Primary:
		EquipmentName = StorageSubsystem->CacheLoadout->Loadouts[LoadoutIndex].Primary;
		break;
	case EEquipmentType::Secondary:
		EquipmentName = StorageSubsystem->CacheLoadout->Loadouts[LoadoutIndex].Secondary;
		break;
	case EEquipmentType::Melee:
		EquipmentName = StorageSubsystem->CacheLoadout->Loadouts[LoadoutIndex].Melee;
		break;
	case EEquipmentType::Throwing:
		EquipmentName = StorageSubsystem->CacheLoadout->Loadouts[LoadoutIndex].Throwing;
		break;
	}

	return EquipmentName;
}

void AHumanCharacter::AimButtonPressed(const FInputActionValue& Value)
{
	if (CombatComp == nullptr || !ASC) return;
	
	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());

	if (CombatComp->GetCurWeapon())
	{
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_EQUIPMENT_AIM));
	}
	else if (CombatComp->CurEquipmentType == EEquipmentType::Melee)
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
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bIsAdjustEquipmentSocketTransform) return;
	}

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
	
	switch (CombatComp->CurEquipmentType)
	{
	case EEquipmentType::Primary:
	case EEquipmentType::Secondary:
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_WEAPON_FIRE));
		break;
	case EEquipmentType::Melee:
		{
			FGameplayEventData Payload;
			Payload.EventMagnitude = 1.f; // 轻击
			if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
			if (BasePlayerState && BasePlayerState->InputType == ECommonInputType::Gamepad)
			{
				Payload.EventMagnitude = 2.f;  // 手柄开火键绑的是轻击
			}
			ASC->HandleGameplayEvent(TAG_EVENT_MELEE_ATTACK, &Payload);
		}
		break;
	case EEquipmentType::Throwing:
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_THROWING_THROW));
		break;
	}
}

void AHumanCharacter::FireButtonReleased(const FInputActionValue& Value)
{
	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
	
	switch (CombatComp->CurEquipmentType)
	{
	case EEquipmentType::Primary:
	case EEquipmentType::Secondary:
		{
			FGameplayTagContainer AbilityTags;
			AbilityTags.AddTag(TAG_ABILITY_WEAPON_FIRE);
			ASC->CancelAbilities(&AbilityTags);
		}
		break;
	case EEquipmentType::Melee:
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
	Payload.EventMagnitude = static_cast<float>(CombatComp->CurEquipmentType); 
	ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_DROP, &Payload);
	
	bCanSwitchLoadout = false;
}

void AHumanCharacter::SwapLastEquipmentButtonPressed(const FInputActionValue& Value)
{
	if (!CombatComp) return;
	
	if (CombatComp->CurEquipmentType != CombatComp->DesiredEquipmentType)
    {
        // 玩家在切枪中途又按了切枪，把目标改回我原本手里的这把枪（TargetEquipmentType技能触发就会改变，CurEquipmentType切出动画完成才会改变）
        SendSwapEquipmentEvent(CombatComp->CurEquipmentType);
    }
    else
    {
        // 正常状态，没在切枪，直接切向上一把武器
        SendSwapEquipmentEvent(CombatComp->LastEquipmentType);
    }
}

void AHumanCharacter::SwapBetweenPrimarySecondaryEquipmentButtonPressed(const FInputActionValue& Value)
{
	if (CombatComp)
	{
		if (AWeapon* PrimaryWeapon = Cast<AWeapon>(CombatComp->GetEquipmentByType(EEquipmentType::Primary)))
		{
			EEquipmentType EquipmentType = EEquipmentType::Primary;
			if (CombatComp->CurEquipmentType == EEquipmentType::Primary ||
				CombatComp->CurEquipmentType != EEquipmentType::Primary && PrimaryWeapon->IsEmpty())
			{
				EquipmentType = EEquipmentType::Secondary;
			}
			SendSwapEquipmentEvent(EquipmentType);
		}
	}
}

void AHumanCharacter::SendSwapEquipmentEvent(EEquipmentType TargetEquipmentType)
{
	// 如果玩家想切的目标，就是他现在使用的或正在努力切过去的装备，直接返回
	if (CombatComp && CombatComp->DesiredEquipmentType == TargetEquipmentType) return;
	
	if (ASC)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		Payload.EventMagnitude = static_cast<float>(TargetEquipmentType); 
		ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_SWAP, &Payload);
	}
}

void AHumanCharacter::ClientSwapEquipmentWhenPickupFailed_Implementation(EEquipmentType FallbackEquipmentType)
{
	if (bIsDead || CombatComp == nullptr) return;
	if (CombatComp->CurEquipmentType == FallbackEquipmentType) return; // 如果本地客户端已经通过预测切到了目标武器，就不需要再切一次了，防止打断动画
	
	SendSwapEquipmentEvent(CombatComp->GetLastEquipment() ? CombatComp->LastEquipmentType : EEquipmentType::Melee);
}

// 装备检测到的重叠装备
void AHumanCharacter::EquipOverlappingEquipment_Server(AEquipment* Equipment)
{
	if (bIsDead || Equipment == nullptr || Equipment->GetOwner() || CombatComp == nullptr) return;
	if (CombatComp->HasEquippedEquipment(Equipment->EquipmentType)) return;
	if (CombatComp->MeleeEquipment) // HACK 排除玩家刚重生还没有装备好武器（近战武器装备后无法丢弃）
	{
		CombatComp->EquipEquipment(Equipment);
		
		CombatComp->ClientPlayEquipSound();
	}
}

// 给予补给箱装备
void AHumanCharacter::ServerGivePickupEquipment_Implementation(APickupEquipment* PickupEquipment)
{
	if (bIsDead || CombatComp == nullptr || PickupEquipment == nullptr) return;

	AEquipment* Equipment = PickupEquipment->Equipment;
	if (Equipment == nullptr) return;

	if (Equipment->EquipmentType !=EEquipmentType::Primary && Equipment->EquipmentType != EEquipmentType::Secondary)
	{
		UE_LOG(LogTemp, Error, TEXT("PickupEquipment EquipmentType is not Primary or Secondary"));
		return;
	}
	
	// 丢弃旧装备
	FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
	FGameplayEventData Payload;
	Payload.EventMagnitude = static_cast<float>(Equipment->EquipmentType);
	Payload.OptionalObject = PickupEquipment;
	ASC->HandleGameplayEvent(TAG_EVENT_EQUIPMENT_DROP, &Payload);
}

void AHumanCharacter::TrySwitchLoadout()
{
	if (CombatComp == nullptr) return;
	
	if (bCanSwitchLoadout)
	{
		CombatComp->LocalDestroyEquipments();
		CombatComp->ServerDestroyEquipments();

		ApplyLoadout();
	}
	else
	{
		NOTIFY(this, C_WHITE, LOCTEXT("LoadoutApplyTip", "Loadout will apply in the next respawn"));
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
		HandleDead();
	}
}

void AHumanCharacter::MutationDead(bool bNeedSpawn, ESpawnMutantReason SpawnMutantReason)
{
	bIsDead = true;
	OnRep_bIsDead(); // 服务端本地主动调用一次死亡表现

	if (HasAuthority() && bNeedSpawn)
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this, SpawnMutantReason]() {
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
	OnRep_bIsDead();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
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
	OnRep_bIsDead();

	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			if (ATeamDeadMatchMode* TeamDeadMatchMode = GetWorld()->GetAuthGameMode<ATeamDeadMatchMode>())
			{
				TeamDeadMatchMode->HumanRespawn(this, Controller);
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AHumanCharacter::HandleDead()
{
	if (CombatComp)
	{
		// 直接在服务端丢弃主武器不走GA
		if (HasAuthority())
		{
			if (AEquipment* PrimaryEquip = CombatComp->GetEquipmentByType(EEquipmentType::Primary))
			{
				PrimaryEquip->ClearAbilities(ASC);
				PrimaryEquip->Drop(); 
			}
		}
		
		CombatComp->LocalDestroyEquipments(false);
	}

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController) DisableInput(BaseController);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AHumanCharacter::BecomeImmune()
{
	bIsImmune = true;
	OnRep_bIsImmune();
}

void AHumanCharacter::OnRep_bIsImmune()
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->OnOverheadWidgetNeedUpdate.Broadcast();
			
			if (bIsImmune)
			{
				UISubsystem->OnBeImmune.Broadcast();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
