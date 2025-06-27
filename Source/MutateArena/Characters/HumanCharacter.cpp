#include "HumanCharacter.h"

#include "DataRegistryId.h"
#include "DataRegistrySubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "..\Equipments\Throwing.h"
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
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatStateType.h"
#include "Components/CrosshairComponent.h"
#include "Components/RecoilComponent.h"
#include "Data/CharacterType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/InputAsset.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "AHumanCharacter"

AHumanCharacter::AHumanCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	RecoilComponent = CreateDefaultSubobject<URecoilComponent>(TEXT("RecoilComponent"));
	CrosshairComponent = CreateDefaultSubobject<UCrosshairComponent>(TEXT("CrosshairComponent"));

	BloodColor = C_RED;

	Tags.Add(TAG_CHARACTER_HUMAN);
}

void AHumanCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, DefaultPrimary);
	DOREPLIFETIME(ThisClass, DefaultSecondary);
	DOREPLIFETIME(ThisClass, DefaultMelee);
	DOREPLIFETIME(ThisClass, DefaultThrowing);
	DOREPLIFETIME(ThisClass, bIsImmune);
}

void AHumanCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CombatComponent && CombatComponent->HumanCharacter == nullptr)
	{
		CombatComponent->HumanCharacter = this;
	}
	if (RecoilComponent && RecoilComponent->HumanCharacter == nullptr)
	{
		RecoilComponent->HumanCharacter = this;
	}
	if (CrosshairComponent && CrosshairComponent->HumanCharacter == nullptr)
	{
		CrosshairComponent->HumanCharacter = this;
	}
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
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->AimAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->AimAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->FireAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->ReloadAction, ETriggerEvent::Triggered, this, &ThisClass::ReloadButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->DropAction, ETriggerEvent::Triggered, this, &ThisClass::DropButtonPressed);

		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->SwapPrimaryEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapPrimaryEquipmentButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->SwapSecondaryEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapSecondaryEquipmentButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->SwapMeleeEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapMeleeEquipmentButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->SwapThrowingEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapThrowingEquipmentButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->SwapLastEquipmentAction, ETriggerEvent::Triggered, this, &ThisClass::SwapLastEquipmentButtonPressed);
	}
}

void AHumanCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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
	if (CombatComponent == nullptr) return;

	TSubclassOf<AEquipment> PrimaryClass = nullptr;
	TSubclassOf<AEquipment> SecondaryClass = nullptr;
	TSubclassOf<AEquipment> MeleeClass = nullptr;
	TSubclassOf<AEquipment> ThrowingClass = nullptr;

	FName PrimaryName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Primary)));
	FName SecondaryName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Secondary)));
	FName MeleeName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Melee)));
	FName ThrowingName = FName(ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Throwing)));

	// 近战模式只生成近战装备
	if (MeleeGameState == nullptr) MeleeGameState = GetWorld()->GetGameState<AMeleeGameState>();
	if (MeleeGameState)
	{
		PrimaryName = FName();
		SecondaryName = FName();
		ThrowingName = FName();
	}

	// 包括Standalone
#if UE_EDITOR
	if (GetDefault<UDevSetting>()->bUseEquipmentSettings)
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

	if (PrimaryClass && PrimaryClass->IsChildOf<AWeapon>())
	{
		DefaultPrimary = GetWorld()->SpawnActor<AWeapon>(PrimaryClass);
		CombatComponent->LocalEquipEquipment(DefaultPrimary);
		CombatComponent->LocalSwapEquipment(EEquipmentType::Primary);
	}
	if (SecondaryClass && SecondaryClass->IsChildOf<AWeapon>())
	{
		DefaultSecondary = GetWorld()->SpawnActor<AWeapon>(SecondaryClass);
		CombatComponent->LocalEquipEquipment(DefaultSecondary);
	}
	if (MeleeClass && MeleeClass->IsChildOf<AMelee>())
	{
		DefaultMelee = GetWorld()->SpawnActor<AMelee>(MeleeClass);
		CombatComponent->LocalEquipEquipment(DefaultMelee);

		// 近战模式默认切到近战武器
		if (MeleeGameState == nullptr) MeleeGameState = GetWorld()->GetGameState<AMeleeGameState>();
		if (MeleeGameState)
		{
			CombatComponent->LocalSwapEquipment(EEquipmentType::Melee);
		}
	}
	if (ThrowingClass && ThrowingClass->IsChildOf<AThrowing>())
	{
		DefaultThrowing = GetWorld()->SpawnActor<AThrowing>(ThrowingClass);
		CombatComponent->LocalEquipEquipment(DefaultThrowing);
	}
}

// 获取装备的名字
EEquipmentName AHumanCharacter::GetEquipmentName(int32 LoadoutIndex, EEquipmentType EquipmentType)
{
	EEquipmentName EquipmentName = EEquipmentName::None;

	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (StorageSubsystem == nullptr || StorageSubsystem->CacheLoadout->Loadouts.IsEmpty())
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

void AHumanCharacter::OnRep_DefaultPrimary()
{
	if (CombatComponent)
	{
		CombatComponent->LocalEquipEquipment(DefaultPrimary);
		CombatComponent->LocalSwapEquipment(EEquipmentType::Primary);
	}
}

void AHumanCharacter::OnRep_DefaultSecondary()
{
	if (CombatComponent)
	{
		CombatComponent->LocalEquipEquipment(DefaultSecondary);
	}
}

void AHumanCharacter::OnRep_DefaultMelee()
{
	if (CombatComponent)
	{
		CombatComponent->LocalEquipEquipment(DefaultMelee);
		
		if (MeleeGameState == nullptr) MeleeGameState = GetWorld()->GetGameState<AMeleeGameState>();
		if (MeleeGameState)
		{
			CombatComponent->LocalSwapEquipment(EEquipmentType::Melee);
		}
	}
}

void AHumanCharacter::OnRep_DefaultThrowing()
{
	if (CombatComponent)
	{
		CombatComponent->LocalEquipEquipment(DefaultThrowing);
	}
}

void AHumanCharacter::AimButtonPressed(const FInputActionValue& Value)
{
	if (CombatComponent == nullptr) return;

	if (CombatComponent->GetUsingWeapon())
	{
		CombatComponent->SetAiming(true);
	}
	else if (CombatComponent->CurEquipmentType == EEquipmentType::Melee)
	{
		CombatComponent->MeleeAttack(ECombatState::HeavyAttacking);
	}
}

void AHumanCharacter::AimButtonReleased(const FInputActionValue& Value)
{
	if (CombatComponent && CombatComponent->GetUsingWeapon())
	{
		CombatComponent->SetAiming(false);
	}
}

void AHumanCharacter::FireButtonPressed(const FInputActionValue& Value)
{
	if (CombatComponent == nullptr) return;

	bCanSwitchLoadout = false;

	switch (CombatComponent->CurEquipmentType)
	{
	case EEquipmentType::Primary:
		CombatComponent->StartFire();
	case EEquipmentType::Secondary:
		CombatComponent->StartFire();
	case EEquipmentType::Melee:
		CombatComponent->MeleeAttack(ECombatState::LightAttacking);
	case EEquipmentType::Throwing:
		CombatComponent->Throw();
	}
}

void AHumanCharacter::FireButtonReleased(const FInputActionValue& Value)
{
	if (CombatComponent == nullptr) return;

	switch (CombatComponent->CurEquipmentType)
	{
	case EEquipmentType::Primary:
		CombatComponent->StopFire();
	case EEquipmentType::Secondary:
		CombatComponent->StopFire();
	case EEquipmentType::Melee:
		CombatComponent->bIsCombo = false;
		CombatComponent->ServerSetIsCombo(false);
	}
}

void AHumanCharacter::ReloadButtonPressed(const FInputActionValue& Value)
{
	if (CombatComponent && CombatComponent->GetUsingWeapon())
	{
		CombatComponent->Reload();
	}
}

void AHumanCharacter::DropButtonPressed(const FInputActionValue& Value)
{
	// 只有主副武器可以丢弃
	if (CombatComponent && CombatComponent->GetUsingWeapon())
	{
		CombatComponent->DropEquipment(CombatComponent->CurEquipmentType);

		bCanSwitchLoadout = false;
	}
}

void AHumanCharacter::SwapPrimaryEquipmentButtonPressed()
{
	if (CombatComponent) CombatComponent->SwapEquipment(EEquipmentType::Primary);
}

void AHumanCharacter::SwapSecondaryEquipmentButtonPressed()
{
	if (CombatComponent) CombatComponent->SwapEquipment(EEquipmentType::Secondary);
}

void AHumanCharacter::SwapMeleeEquipmentButtonPressed()
{
	if (CombatComponent) CombatComponent->SwapEquipment(EEquipmentType::Melee);
}

void AHumanCharacter::SwapThrowingEquipmentButtonPressed()
{
	if (CombatComponent) CombatComponent->SwapEquipment(EEquipmentType::Throwing);
}

void AHumanCharacter::SwapLastEquipmentButtonPressed(const FInputActionValue& Value)
{
	if (CombatComponent) CombatComponent->SwapEquipment(CombatComponent->LastEquipmentType);
}

void AHumanCharacter::OnServerDropEquipment()
{
	if (bIsDead || CombatComponent == nullptr) return;

	// 如果脚下有同类型武器直接拾取使用
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, AEquipment::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (AEquipment* Equipment = Cast<AEquipment>(OverlappingActor))
		{
			if (CombatComponent->CurEquipmentType == Equipment->EquipmentType)
			{
				CombatComponent->MulticastReplaceCurEquipment(Equipment);
			}

			return;
		}
	}
	
	// 否则切换至其它装备
	CombatComponent->MulticastSwapEquipment2(CombatComponent->GetLastEquipment() ? CombatComponent->LastEquipmentType : EEquipmentType::Melee);
}

// 装备检测到的重叠装备
void AHumanCharacter::EquipOverlappingEquipment(AEquipment* Equipment)
{
	if (bIsDead || Equipment == nullptr || Equipment->GetOwner() || CombatComponent == nullptr) return;
	if (CombatComponent->HasEquippedThisTypeEquipment(Equipment->EquipmentType)) return;
	if (CombatComponent->MeleeEquipment) // 排除玩家刚重生还没有装备好武器（近战武器装备后无法丢弃）
	{
		CombatComponent->MulticastEquipEquipment2(Equipment);
	}
}

// 给予补给箱装备
void AHumanCharacter::ServerGivePickupEquipment_Implementation(APickupEquipment* PickupEquipment)
{
	if (bIsDead || CombatComponent == nullptr || PickupEquipment == nullptr) return;

	AEquipment* Equipment = PickupEquipment->Equipment;
	if (Equipment == nullptr) return;

	// 丢弃旧装备
	CombatComponent->MulticastDropEquipment2(Equipment->EquipmentType);

	// 装备类型使用中，替换
	if (CombatComponent->CurEquipmentType == Equipment->EquipmentType)
	{
		CombatComponent->MulticastReplaceCurEquipment(Equipment);
	}
	// 装备类型非使用中，装备
	else
	{
		CombatComponent->MulticastEquipEquipment2(Equipment);
	}

	PickupEquipment->Destroy();
}

void AHumanCharacter::TrySwitchLoadout()
{
	if (CombatComponent == nullptr) return;
	
	if (bCanSwitchLoadout)
	{
		CombatComponent->LocalDestroyEquipments();
		CombatComponent->ServerDestroyEquipments();

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

void AHumanCharacter::MulticastMutationDead_Implementation(bool bNeedSpawn, ESpawnMutantReason SpawnMutantReason)
{
	HandleDead();

	if (HasAuthority() && bNeedSpawn)
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this, SpawnMutantReason]() {
			if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
			if (MutationMode)
			{
				MutationMode->Mutate(this, Controller, SpawnMutantReason);
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AHumanCharacter::MulticastMeleeDead_Implementation()
{
	HandleDead();

	// 重生
	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			if (MeleeMode == nullptr) MeleeMode = GetWorld()->GetAuthGameMode<AMeleeMode>();
			if (MeleeMode)
			{
				MeleeMode->HumanRespawn(this, Controller);
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AHumanCharacter::MulticastTeamDeadMatchDead_Implementation()
{
	HandleDead();

	// 重生
	if (HasAuthority())
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			if (TeamDeadMatchMode == nullptr) TeamDeadMatchMode = GetWorld()->GetAuthGameMode<ATeamDeadMatchMode>();
			if (TeamDeadMatchMode)
			{
				TeamDeadMatchMode->HumanRespawn(this, Controller);
			}
		});
		GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AHumanCharacter::HandleDead()
{
	bIsDead = true;

	if (CombatComponent)
	{
		CombatComponent->LocalDropEquipment(EEquipmentType::Primary);
		CombatComponent->LocalDestroyEquipments();
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

void AHumanCharacter::OnInteractMutantSuccess(class AMutantCharacter* MutantCharacter)
{
	bIsImmune = true;

	if (AMutationController* InteractController = Cast<AMutationController>(Controller))
	{
		InteractController->OnBeImmune.Broadcast();
	}

	ServerOnImmune(MutantCharacter);
}

void AHumanCharacter::ServerOnImmune_Implementation(class AMutantCharacter* MutantCharacter)
{
	bIsImmune = true;
}

void AHumanCharacter::OnRep_bIsImmune()
{
}

#undef LOCTEXT_NAMESPACE
