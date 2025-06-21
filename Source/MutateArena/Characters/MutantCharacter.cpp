#include "MutantCharacter.h"

#include "DataRegistrySubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HumanCharacter.h"
#include "MetaSoundSource.h"
#include "MutateArena/Characters/AnimInstance_Mutant.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
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
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "AMutantCharacter"

AMutantCharacter::AMutantCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	BloodColor = C_GREEN;

	MutantState = EMutantState::Ready;

	RightHandCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RightHandCapsule"));
	RightHandCapsule->SetupAttachment(GetMesh(), TEXT("RightHandCapsule"));
	RightHandCapsule->SetGenerateOverlapEvents(true);
	RightHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightHandCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	RightHandCapsule->SetCollisionResponseToChannel(ECC_TEAM1_MESH, ECollisionResponse::ECR_Overlap);
	RightHandCapsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnRightHandCapsuleOverlap);

	LeftHandCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("LeftHandCapsule"));
	LeftHandCapsule->SetupAttachment(GetMesh(), TEXT("LeftHandCapsule"));
	LeftHandCapsule->SetGenerateOverlapEvents(true);
	LeftHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftHandCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LeftHandCapsule->SetCollisionResponseToChannel(ECC_TEAM1_MESH, ECollisionResponse::ECR_Overlap);
	LeftHandCapsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnLeftHandCapsuleOverlap);

	Tags.Add(TAG_CHARACTER_MUTANT);
}

void AMutantCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bSuckedDry);
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

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->LightAttackAction, ETriggerEvent::Started, this, &ThisClass::LightAttackButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->LightAttackAction, ETriggerEvent::Completed, this, &ThisClass::LightAttackButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->HeavyAttackAction, ETriggerEvent::Started, this, &ThisClass::HeavyAttackButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->HeavyAttackAction, ETriggerEvent::Completed, this, &ThisClass::HeavyAttackButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->SkillAction, ETriggerEvent::Triggered, this, &ThisClass::SkillButtonPressed);
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

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(SkillAbility));

		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->CharacterAsset)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AssetSubsystem->CharacterAsset->MutantChangeAbility));
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AssetSubsystem->CharacterAsset->MutantRestoreAbility));

			if (SpawnMutantReason != ESpawnMutantReason::SelectMutant)
			{
				AbilitySystemComponent->TryActivateAbilityByClass(AssetSubsystem->CharacterAsset->MutantChangeAbility);
			}
		}
	}
}

void AMutantCharacter::OnAbilitySystemComponentInit()
{
	Super::OnAbilitySystemComponentInit();

	if (AbilitySystemComponent && AttributeSetBase && IsLocallyControlled())
	{
		FGameplayTag SkillCooldownTag = FGameplayTag::RequestGameplayTag(TAG_MUTANT_SKILL_CD);
		AbilitySystemComponent->RegisterGameplayTagEvent(
			SkillCooldownTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnLocalSkillCooldownTagChanged);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			AttributeSetBase->GetCharacterLevelAttribute()).AddUObject(this, &ThisClass::OnLocalCharacterLevelChanged);
	}
}

// 更新skill UI
void AMutantCharacter::OnLocalSkillCooldownTagChanged(FGameplayTag GameplayTag, int32 TagCount)
{
	if (MutationController == nullptr) MutationController = Cast<AMutationController>(Controller);
	if (MutationController)
	{
		MutationController->ShowHUDSkill(TagCount == 0 && GetCharacterLevel() >= 2.f);
	}
}

// 更新skill UI
void AMutantCharacter::OnLocalCharacterLevelChanged(const FOnAttributeChangeData& Data)
{
	if (MutationController == nullptr) MutationController = Cast<AMutationController>(Controller);
	if (MutationController && AbilitySystemComponent)
	{
		FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TAG_MUTANT_SKILL_CD);
		MutationController->ShowHUDSkill(AbilitySystemComponent->GetTagCount(Tag) == 0 && Data.NewValue > 2.f);
	}
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
	if (AbilitySystemComponent && GetCharacterLevel() >= 2.f)
	{
		AbilitySystemComponent->TryActivateAbilityByClass(SkillAbility);
	}
}

void AMutantCharacter::Destroyed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}

	Super::Destroyed();
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

// 回复血量
void AMutantCharacter::ActivateRestoreAbility()
{
	if (AbilitySystemComponent && AssetSubsystem && AssetSubsystem->CharacterAsset)
	{
		bHasActivateRestoreAbility = true;
		AbilitySystemComponent->TryActivateAbilityByClass(AssetSubsystem->CharacterAsset->MutantRestoreAbility);
	}
}

// 停止回复血量
void AMutantCharacter::EndRestoreAbility()
{
	if (StillTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(StillTimerHandle);
	}

	if (bHasActivateRestoreAbility)
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AbilitySystemComponent && AssetSubsystem && AssetSubsystem->CharacterAsset)
		{
			if (FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromClass(AssetSubsystem->CharacterAsset->MutantRestoreAbility))
			{
				bHasActivateRestoreAbility = false;

				AbilitySystemComponent->CancelAbility(Spec->Ability);
			}
		}
	}
}

void AMutantCharacter::LightAttackButtonPressed(const FInputActionValue& Value)
{
	if (MutantState != EMutantState::Ready) return;

	LocalLightAttack();
	ServerLightAttack();
}

void AMutantCharacter::ServerLightAttack_Implementation()
{
	MulticastLightAttack();
}

void AMutantCharacter::MulticastLightAttack_Implementation()
{
	if (!IsLocallyControlled())
	{
		LocalLightAttack();
	}
}

void AMutantCharacter::LocalLightAttack()
{
	if (AnimInstance_Mutant == nullptr) AnimInstance_Mutant = Cast<UAnimInstance_Mutant>(GetMesh()->GetAnimInstance());
	if (AnimInstance_Mutant)
	{
		bIsCombo = true;
		ServerSetIsCombo(true);
		MutantState = EMutantState::LightAttacking;
		AnimInstance_Mutant->Montage_Play(LightAttackMontage);

		// 动画结束后，重置状态
		FOnMontageEnded OnMontageEnded;
		OnMontageEnded.BindWeakLambda(this, [this](UAnimMontage* AnimMontage, bool bInterrupted)
		{
			MutantState = EMutantState::Ready;
			RightHandHitEnemies.Empty();
			LeftHandHitEnemies.Empty();
			
			if (IsLocallyControlled() && bIsCombo)
			{
				LightAttackButtonPressed(FInputActionValue());
			}
		});
		AnimInstance_Mutant->Montage_SetEndDelegate(OnMontageEnded, LightAttackMontage);
	}
}

void AMutantCharacter::LightAttackButtonReleased(const FInputActionValue& Value)
{
	bIsCombo = false;
	ServerSetIsCombo(false);
}

void AMutantCharacter::HeavyAttackButtonPressed(const FInputActionValue& Value)
{
	if (MutantState != EMutantState::Ready) return;

	LocalHeavyAttack();
	ServerHeavyAttack();
}

void AMutantCharacter::ServerHeavyAttack_Implementation()
{
	MulticastHeavyAttack();
}

void AMutantCharacter::MulticastHeavyAttack_Implementation()
{
	if (!IsLocallyControlled())
	{
		LocalHeavyAttack();
	}
}

void AMutantCharacter::LocalHeavyAttack()
{
	if (AnimInstance_Mutant == nullptr) AnimInstance_Mutant = Cast<UAnimInstance_Mutant>(GetMesh()->GetAnimInstance());
	if (AnimInstance_Mutant)
	{
		MutantState = EMutantState::HeavyAttacking;
		AnimInstance_Mutant->Montage_Play(HeavyAttackMontage);

		// 动画结束后，重置状态
		FOnMontageEnded OnMontageEnded;
		OnMontageEnded.BindWeakLambda(this, [this](UAnimMontage* AnimMontage, bool bInterrupted)
		{
			MutantState = EMutantState::Ready;
			RightHandHitEnemies.Empty();
			LeftHandHitEnemies.Empty();
		});
		AnimInstance_Mutant->Montage_SetEndDelegate(OnMontageEnded, HeavyAttackMontage);
	}
}

void AMutantCharacter::HeavyAttackButtonReleased(const FInputActionValue& Value)
{
}

void AMutantCharacter::ServerSetIsCombo_Implementation(bool TempBIsCombo)
{
	MulticastSetIsCombo(TempBIsCombo);
}

void AMutantCharacter::MulticastSetIsCombo_Implementation(bool TempBIsCombo)
{
	if (!IsLocallyControlled())
	{
		bIsCombo = TempBIsCombo;
	}
}

void AMutantCharacter::AttackFirstSectionEnd()
{
	if (bIsCombo)
	{
		RightHandHitEnemies.Empty();
		LeftHandHitEnemies.Empty();
	}
	else
	{
		if (AnimInstance_Mutant == nullptr) AnimInstance_Mutant = Cast<UAnimInstance_Mutant>(GetMesh()->GetAnimInstance());
		if (AnimInstance_Mutant)
		{
			AnimInstance_Mutant->Montage_Stop(.25, LightAttackMontage);
		}
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

		float Damage = MutantState == EMutantState::LightAttacking ? LightAttackDamage : HeavyAttackDamage;

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

		float Damage = MutantState == EMutantState::LightAttacking ? LightAttackDamage : HeavyAttackDamage;

		DropBlood(OverlappedComponent, OtherActor, OtherComp, Damage);

		if (IsLocallyControlled())
		{
			ServerApplyDamage(OtherActor, Damage);
		}
	}
}

void AMutantCharacter::ServerApplyDamage_Implementation(AActor* OtherActor, float Damage)
{
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
		if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
		if (MutationMode)
		{
			MutationMode->GetInfect(DamagedCharacter, DamagedController, this, BaseController, MutantState);
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

void AMutantCharacter::MulticastDead_Implementation(bool bKilledByMelee)
{
	bIsDead = true;
	
	EndRestoreAbility();
	
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController == nullptr) DisableInput(BaseController);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RightHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftHandCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this, bKilledByMelee]() {
		this->MutantRespawn(bKilledByMelee);
	});
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 3.f, false);
}

void AMutantCharacter::MutantRespawn(bool bKilledByMelee)
{
	// 移除绑定（没有写到UnPossessed里是因为，Character不销毁的话不触发本地客户端的UnPossessed）
	RemoveMappingContext();

	// 重生
	if (HasAuthority())
	{
		if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
		if (MutationMode)
		{
			MutationMode->MutantRespawn(this, BaseController, bKilledByMelee);
		}
	}
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

bool AMutantCharacter::CanInteract()
{
	return bIsDead && !bSuckedDry;
}

void AMutantCharacter::OnInteract(ABaseCharacter* BaseCharacter)
{
	if (IInteractableTarget* Interactor = Cast<IInteractableTarget>(BaseCharacter))
	{
		// 交互目标本地立即响应交互事件
		SetIsSuckedDry(bSuckedDry);

		// 通知交互者交互成功
		Interactor->OnInteractMutantSuccess(this);
	}
}

void AMutantCharacter::OnInteractOnServer()
{
	SetIsSuckedDry(true);
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

void AMutantCharacter::OnInteractMutantSuccess(AMutantCharacter* MutantCharacter)
{
	ServerOnSuck(MutantCharacter);
}

void AMutantCharacter::ServerOnSuck_Implementation(AMutantCharacter* MutantCharacter)
{
	if (AMutationPlayerState* MutationPlayerState = GetPlayerState<AMutationPlayerState>())
	{
		MutationPlayerState->SetRage(MutationPlayerState->Rage + 2000.f);
	}
}

#undef LOCTEXT_NAMESPACE
