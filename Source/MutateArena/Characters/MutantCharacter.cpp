#include "MutantCharacter.h"

#include "DataRegistrySubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HumanCharacter.h"
#include "MetaSoundSource.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AutoHostComponent.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/Abilities/GameplayAbilityBase.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/DamageTypeMutantDamage.h"
#include "MutateArena/GameModes/MutationMode.h"
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/MutationPlayerState.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/InputAsset.h"
#include "Data/CharacterAsset.h"
#include "MutateArena/Effects/BloodCollision.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"

#define LOCTEXT_NAMESPACE "AMutantCharacter"

AMutantCharacter::AMutantCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	BloodColor = C_GREEN;

	RightHandCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RightHandCapsule"));
	RightHandCapsule->SetupAttachment(GetMesh(), SOCKET_HAND_RIGHT);
	RightHandCapsule->SetGenerateOverlapEvents(true);
	RightHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightHandCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	RightHandCapsule->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Overlap);
	RightHandCapsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnRightHandCapsuleOverlap);

	LeftHandCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("LeftHandCapsule"));
	LeftHandCapsule->SetupAttachment(GetMesh(), SOCKET_HAND_LEFT);
	LeftHandCapsule->SetGenerateOverlapEvents(true);
	LeftHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftHandCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LeftHandCapsule->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Overlap);
	LeftHandCapsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnLeftHandCapsuleOverlap);

	Tags.Add(TAG_CHARACTER_MUTANT);
}

void AMutantCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bSuckedDry);
	DOREPLIFETIME(ThisClass, bIsLightAttack);
	DOREPLIFETIME(ThisClass, bKilledByMelee);
}

void AMutantCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AMutantCharacter::BeginPlay()
{
	Super::BeginPlay();

	FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(MutantCharacterName));
	FDataRegistryId DataRegistryId(DR_MUTANT_CHARACTER_MAIN, FName(EnumValue));
	if (const FMutantCharacterMain* MutantCharacterMain = UDataRegistrySubsystem::Get()->GetCachedItem<FMutantCharacterMain>(DataRegistryId))
	{
		LightAttackDamage = MutantCharacterMain->LightAttackDamage;
		HeavyAttackDamage = MutantCharacterMain->HeavyAttackDamage;
	}

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::MutantReceiveDamage);
	}

	UGameplayStatics::SpawnSoundAttached(MutantBornSound, GetMesh());
}

void AMutantCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->InputAsset == nullptr) return;

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(AssetSubsystem->InputAsset->MutantMappingContext, 200);
		}
	}

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(AssetSubsystem->InputAsset->LookMouseAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(AssetSubsystem->InputAsset->LookStickAction, ETriggerEvent::Triggered, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(AssetSubsystem->InputAsset->LightAttackAction, ETriggerEvent::Started, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		EIC->BindAction(AssetSubsystem->InputAsset->HeavyAttackAction, ETriggerEvent::Started, AutoHostComp, &UAutoHostComponent::UpdateActiveTime);
		
		EIC->BindAction(AssetSubsystem->InputAsset->LightAttackAction, ETriggerEvent::Started, this, &ThisClass::LightAttackButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->LightAttackAction, ETriggerEvent::Completed, this, &ThisClass::LightAttackButtonReleased);
		EIC->BindAction(AssetSubsystem->InputAsset->HeavyAttackAction, ETriggerEvent::Started, this, &ThisClass::HeavyAttackButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->HeavyAttackAction, ETriggerEvent::Completed, this, &ThisClass::HeavyAttackButtonReleased);
		EIC->BindAction(AssetSubsystem->InputAsset->SkillAction, ETriggerEvent::Triggered, this, &ThisClass::SkillButtonPressed);
	}
}

void AMutantCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 调试用 自动行走
	// if (IsLocallyControlled())
	// {
	// 	AddMovementInput(GetActorForwardVector(), 1);
	// }
}

void AMutantCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (ASC)
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->CharacterAsset)
		{
			for (TSubclassOf<UGameplayAbility> AbilityClass : AssetSubsystem->CharacterAsset->MutantDefaultAbilities)
			{
				FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
				ASC->GiveAbility(AbilitySpec);
			}
			
			if (SpawnMutantReason != ESpawnMutantReason::SelectMutant)
			{
				FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
				ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_MUTANT_CHANGE));
			}
		}
		
		ASC->GiveAbility(FGameplayAbilitySpec(SkillAbility, 1, INDEX_NONE, this));
	}
}

void AMutantCharacter::OnASCInit()
{
	Super::OnASCInit();

	if (ASC && AttributeSetBase)
	{
		ASC->RegisterGameplayTagEvent(
			TAG_CD_MUTANT_SKILL, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnLocalSkillCooldownTagChanged);

		ASC->GetGameplayAttributeValueChangeDelegate(
			AttributeSetBase->GetCharacterLevelAttribute()).AddUObject(this, &ThisClass::OnLocalCharacterLevelChanged);
	}

	if (ASC && AttributeSetBase)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetMaxWalkSpeedAttribute()).AddUObject(this, &ThisClass::OnMaxWalkSpeedChanged);
	}
}

// 更新skill UI
void AMutantCharacter::OnLocalSkillCooldownTagChanged(FGameplayTag GameplayTag, int32 TagCount)
{
	if (!IsLocallyControlled()) return;
	
	if (MutationController == nullptr) MutationController = Cast<AMutationController>(Controller);
	if (MutationController)
	{
		MutationController->SetHUDSkill(TagCount == 0 && GetCharacterLevel() >= 2.f);
	}
}

void AMutantCharacter::OnLocalCharacterLevelChanged(const FOnAttributeChangeData& Data)
{
	if (!IsLocallyControlled()) return;
	
	if (MutationController == nullptr) MutationController = Cast<AMutationController>(Controller);
	if (MutationController && ASC)
	{
		MutationController->SetHUDSkill(ASC->GetTagCount(TAG_CD_MUTANT_SKILL) == 0 && Data.NewValue > 2.f);
	}
	UE_LOG(LogTemp, Warning, TEXT("1"));
	if (MutationController)
	{
		UE_LOG(LogTemp, Warning, TEXT("2"));
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(MutationController->GetLocalPlayer()))
		{
			UISubsystem->OnLevelChange.Broadcast(Data.NewValue);
			UE_LOG(LogTemp, Warning, TEXT("3"));
		}
	}
}

void AMutantCharacter::OnMaxWalkSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
	GetCharacterMovement()->MaxWalkSpeedCrouched = Data.NewValue * 0.5f;
}

void AMutantCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	Super::OnHealthChanged(Data);

	// 血量回满，结束回血技能
	if (IsLocallyControlled() && Data.NewValue >= GetMaxHealth())
	{
		EndRestoreAbility();
	}
}

// 处理skill功能
void AMutantCharacter::SkillButtonPressed(const FInputActionValue& Value)
{
	if (ASC && GetCharacterLevel() >= 2.f)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		ASC->TryActivateAbilityByClass(SkillAbility);
	}
}

void AMutantCharacter::Destroyed()
{
	// TODO 是否移到PossessedBy中更合适
	if (ASC)
	{
		if (HasAuthority())
		{
			ASC->ClearAllAbilities();
		}

		FGameplayEffectQuery Query; // 未指定tag则移除全部
		ASC->RemoveActiveEffects(Query);
	}

	Super::Destroyed();
}

void AMutantCharacter::OnLocallyControllerReady()
{
	Super::OnLocallyControllerReady();
}

void AMutantCharacter::MoveStarted(const FInputActionValue& Value)
{
	Super::MoveStarted(Value);

	EndRestoreAbility();
}

void AMutantCharacter::MoveCompleted(const FInputActionValue& Value)
{
	Super::MoveCompleted(Value);

	// 血量不满，激活回血技能。
	if (GetHealth() < GetMaxHealth())
	{
		GetWorldTimerManager().SetTimer(StillTimerHandle, this, &ThisClass::ActivateRestoreAbility, 5.f);
	}
}

// 恢复血量
void AMutantCharacter::ActivateRestoreAbility()
{
	if (ASC)
	{
		bHasActivateRestoreAbility = true;
		
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_ABILITY_MUTANT_RESTORE));
	}
}

// 停止恢复血量
void AMutantCharacter::EndRestoreAbility()
{
	if (StillTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(StillTimerHandle);
	}

	if (bHasActivateRestoreAbility && ASC)
	{
		bHasActivateRestoreAbility = false;
		
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(TAG_ABILITY_MUTANT_RESTORE);
		ASC->CancelAbilities(&CancelTags);
	}
}

void AMutantCharacter::LightAttackButtonPressed(const FInputActionValue& Value)
{
	if (ASC)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		Payload.EventMagnitude = 1.f; // 轻击
		ASC->HandleGameplayEvent(TAG_EVENT_MUTANT_ATTACK_START, &Payload);
	}
}

void AMutantCharacter::LightAttackButtonReleased(const FInputActionValue& Value)
{
	if (ASC)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		ASC->HandleGameplayEvent(TAG_EVENT_MUTANT_ATTACK_RELEASED, &Payload);
	}
}

void AMutantCharacter::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	if (ASC)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		Payload.EventMagnitude = 2.f; // 重击
		ASC->HandleGameplayEvent(TAG_EVENT_MUTANT_ATTACK_START, &Payload);
	}
}

void AMutantCharacter::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
	if (ASC)
	{
		FScopedPredictionWindow PredictionWindow(ASC, IsLocallyControlled() && !HasAuthority());
		FGameplayEventData Payload;
		ASC->HandleGameplayEvent(TAG_EVENT_MUTANT_ATTACK_RELEASED, &Payload);
	}
}

void AMutantCharacter::RightHandAttackBegin()
{
	RightHandCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AMutantCharacter::RightHandAttackEnd()
{
	RightHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMutantCharacter::LeftHandAttackBegin()
{
	LeftHandCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AMutantCharacter::LeftHandAttackEnd()
{
	LeftHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMutantCharacter::OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!RightHandHitEnemies.Contains(OtherActor))
	{
		RightHandHitEnemies.Add(OtherActor);

		float Damage = bIsLightAttack ? LightAttackDamage : HeavyAttackDamage;

		DropBlood(OverlappedComponent, OtherActor, OtherComp, Damage);

		if (IsLocallyControlled())
		{
			ServerApplyDamage(OtherActor, Damage);
		}
	}
}

void AMutantCharacter::OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!LeftHandHitEnemies.Contains(OtherActor))
	{
		LeftHandHitEnemies.Add(OtherActor);
		
		float Damage = bIsLightAttack ? LightAttackDamage : HeavyAttackDamage;

		DropBlood(OverlappedComponent, OtherActor, OtherComp, Damage);

		if (IsLocallyControlled())
		{
			ServerApplyDamage(OtherActor, Damage);
		}
	}
}

void AMutantCharacter::ServerApplyDamage_Implementation(AActor* OtherActor, float Damage)
{
	if (bIsDead) return;
	
	// 对局结束只有人类可以造成伤害
	if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
	if (MutationMode)
	{
		if (MutationMode->GetMatchState() != MatchState::InProgress)
		{
			return;
		}
	}
	
	// 判断Overlap对象不是人类退出（Human变为Mutant时，Team可能未同步到本地，会发生Overlap）。
	AHumanCharacter* DamagedCharacter = Cast<AHumanCharacter>(OtherActor);
	if (DamagedCharacter == nullptr) return;

	// 造成伤害
	UGameplayStatics::ApplyDamage(OtherActor, Damage, Controller, this, UDamageTypeMutantDamage::StaticClass());

	// 造成感染
	if (!DamagedCharacter->bIsDead && !DamagedCharacter->bIsImmune)
	{
		ABaseController* DamagedController = Cast<ABaseController>(DamagedCharacter->Controller);
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
		if (MutationMode)
		{
			MutationMode->GetInfect(DamagedCharacter, DamagedController, this, BaseController);
		}
	}
}

void AMutantCharacter::DropBlood(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, float Damage)
{
	ABaseCharacter* OverlappedCharacter = Cast<ABaseCharacter>(OtherActor);

	if (OverlappedCharacter == nullptr || OverlappedCharacter->BloodEffect == nullptr) return;

	TArray<FHitResult> TraceResults;

	auto Start = OverlappedComponent->GetComponentLocation();
	auto End = OverlappedCharacter->GetActorLocation();

	GetWorld()->SweepMultiByObjectType(
		TraceResults,
		Start,
		End,
		FQuat::Identity,
		FCollisionObjectQueryParams(OverlappedCharacter->GetMesh()->GetCollisionObjectType()),
		FCollisionShape::MakeSphere(10.f)
	);

	// DrawDebugLine(GetWorld(), Start, End, C_YELLOW, true);

	for (auto TraceResult : TraceResults)
	{
		if (TraceResult.GetComponent()->GetUniqueID() == OtherComp->GetUniqueID())
		{
			auto BloodEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				OverlappedCharacter->BloodEffect,
				TraceResult.ImpactPoint,
				TraceResult.ImpactNormal.Rotation()
			);
			if (BloodEffectComponent)
			{
				BloodEffectComponent->SetVariableInt(TEXT("Count"), ULibraryCommon::GetBloodParticleCount(Damage));
				BloodEffectComponent->SetVariableLinearColor(TEXT("Color"), OverlappedCharacter->BloodColor);
				UBloodCollision* CollisionCB = NewObject<UBloodCollision>(this);
				BloodEffectComponent->SetVariableObject(TEXT("CollisionCB"), CollisionCB);
			}
			
			auto BloodSmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				OverlappedCharacter->BloodSmokeEffect,
				TraceResult.ImpactPoint,
				TraceResult.ImpactNormal.Rotation()
			);
			if (BloodSmokeEffectComponent)
			{
				BloodSmokeEffectComponent->SetVariableLinearColor(TEXT("SmokeColor"), OverlappedCharacter->BloodColor);
			}

			break;
		}
	}
}

void AMutantCharacter::MutantReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* AttackerController, AActor* DamageCauser)
{
	if (bIsDead) return;

	if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
	if (MutationMode)
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
		if (BaseController)
		{
			MutationMode->MutantReceiveDamage(this, BaseController, FMath::RoundToInt(Damage), DamageType, AttackerController, DamageCauser);
		}
	}
}

void AMutantCharacter::OnRep_bIsDead()
{
	Super::OnRep_bIsDead();

	if (bIsDead)
	{
		EndRestoreAbility();

		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();

		if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
		if (BaseController) DisableInput(BaseController);

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetEnableGravity(true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RightHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LeftHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindWeakLambda(this, [this]() {
			
			// 被刀死不会销毁角色，Character不销毁的话不触发客户端的ABaseController::OnUnPossess, 主动RemoveMappingContext
			if (bIsDead && bKilledByMelee && IsLocallyControlled()) // bIsDead 判断，防止因延迟导致复活后才执行此操作，误删新角色的输入
			{
				RemoveMappingContext();
			}
			
			if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
			if (MutationMode && HasAuthority())
			{
				MutationMode->MutantRespawn(this, BaseController, bKilledByMelee);
			}
			
		});
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
	}
}

void AMutantCharacter::MutantDead(bool bTempKilledByMelee)
{
	bKilledByMelee = bTempKilledByMelee;
	bIsDead = true;
	OnRep_bIsDead();
}

void AMutantCharacter::RemoveMappingContext()
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->InputAsset == nullptr || AssetSubsystem->InputAsset == nullptr) return;
	
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(BaseController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->BaseMappingContext);
			Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->MutantMappingContext);
			Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->RadialMenuMappingContext);
		}
	}
}

void AMutantCharacter::ServerSelectMutant_Implementation(EMutantCharacterName TempMutantCharacterName)
{
	// 在服务器上保存角色，生成角色时会用
	if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
	if (BasePlayerState)
	{
		BasePlayerState->SetMutantCharacterName(TempMutantCharacterName);
	}

	if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
	if (MutationMode)
	{
		MutationMode->SelectMutant(this, Controller);
	}
}

void AMutantCharacter::MulticastRepel_Implementation(FVector ImpulseVector)
{
	TSharedPtr<FRootMotionSource_ConstantForce> RootMotionSource = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotionSource->Force = ImpulseVector;
	RootMotionSource->Duration = 0.1f;
	RootMotionSource->AccumulateMode = ERootMotionAccumulateMode::Additive;
	GetCharacterMovement()->ApplyRootMotionSource(RootMotionSource);
}

bool AMutantCharacter::CanInteract(ABaseCharacter* Interactor)
{
	if (!bIsDead || bSuckedDry)
	{
		return false;
	}

	if (!Interactor || Interactor->bIsDead)
	{
		return false;
	}

	return true; 
}

void AMutantCharacter::OnInteract_Server(ABaseCharacter* Interactor)
{
	if (!HasAuthority()) return;

	SetIsSuckedDry(true);

	if (AHumanCharacter* Human = Cast<AHumanCharacter>(Interactor))
	{
		Human->BecomeImmune();
	}
	else if (AMutantCharacter* Mutant = Cast<AMutantCharacter>(Interactor))
	{
		Mutant->AddRageOnSuck();
	}
}

void AMutantCharacter::AddRageOnSuck()
{
	if (AMutationPlayerState* MutationPlayerState = GetPlayerState<AMutationPlayerState>())
	{
		MutationPlayerState->SetRage(MutationPlayerState->Rage + 4000.f);
	}
}

void AMutantCharacter::SetIsSuckedDry(bool TempBSuckedDry)
{
	bSuckedDry = TempBSuckedDry;

	SetDeadMaterial();
}

void AMutantCharacter::OnRep_bSuckedDry()
{
	SetDeadMaterial();
}

// 使皮肤变黑
void AMutantCharacter::SetDeadMaterial()
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CharacterAsset)
	{
		GetMesh()->SetOverlayMaterial(AssetSubsystem->CharacterAsset->MI_Overlay_Dead);
	}
}

#undef LOCTEXT_NAMESPACE
