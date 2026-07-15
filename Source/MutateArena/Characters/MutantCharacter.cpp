#include "MutantCharacter.h"

#include "DataRegistrySubsystem.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HumanCharacter.h"
#include "MetaSoundSource.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
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
#include "Components/MAMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/InputAsset.h"
#include "Data/CharacterAsset.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "AMutantCharacter"

AMutantCharacter::AMutantCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	BloodColor = C_GREEN;

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
	
	FString EnumValue = StaticEnum<EMutantCharacterName>()->GetNameStringByValue(static_cast<int64>(MutantCharacterName));
	FDataRegistryId DataRegistryId(DR_MUTANT_CHARACTER_MAIN, FName(EnumValue));
	if (const FMutantCharacterMain* MutantCharacterMain = UDataRegistrySubsystem::Get()->GetCachedItem<FMutantCharacterMain>(DataRegistryId))
	{
		LightAttackDamage = MutantCharacterMain->LightAttackDamage;
		HeavyAttackDamage = MutantCharacterMain->HeavyAttackDamage;
		
		TraceSockets_R = MutantCharacterMain->TraceSockets_R;
		TraceRadius_R = MutantCharacterMain->TraceRadius_R;
		TraceSockets_L = MutantCharacterMain->TraceSockets_L;
		TraceRadius_L = MutantCharacterMain->TraceRadius_L;
	}

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::MutantReceiveDamage);
	}

	UGameplayStatics::SpawnSoundAttached(MutantBornSound, GetMesh());
	
	SetMutantTraceObjectTypes();
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
	
	if (bIsRightHandAttacking)
	{
		PerformHandTrace(TraceSockets_R, RightHandPreviousSocketLocations, true);
	}
	if (bIsLeftHandAttacking)
	{
		PerformHandTrace(TraceSockets_L, LeftHandPreviousSocketLocations, false);
	}
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

	if (MutationController)
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(MutationController->GetLocalPlayer()))
		{
			UISubsystem->OnLevelChange.Broadcast(Data.NewValue);
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
	bIsRightHandAttacking = true;
	RightHandHitEnemies.Empty();
	RightHandPreviousSocketLocations.Empty();
	bRightHandHasHitWall = false;
	
	if (GetMesh())
	{
		for (const FName& SocketName : TraceSockets_R)
		{
			RightHandPreviousSocketLocations.Add(SocketName, GetMesh()->GetSocketLocation(SocketName));
		}
	}
}

void AMutantCharacter::RightHandAttackEnd()
{
	bIsRightHandAttacking = false;
}

void AMutantCharacter::LeftHandAttackBegin()
{
	bIsLeftHandAttacking = true;
	LeftHandHitEnemies.Empty();
	LeftHandPreviousSocketLocations.Empty();
	bLeftHandHasHitWall = false;
	
	if (GetMesh())
	{
		for (const FName& SocketName : TraceSockets_L)
		{
			LeftHandPreviousSocketLocations.Add(SocketName, GetMesh()->GetSocketLocation(SocketName));
		}
	}
}

void AMutantCharacter::LeftHandAttackEnd()
{
	bIsLeftHandAttacking = false;
}

void AMutantCharacter::SetMutantTraceObjectTypes()
{
	MutantObjectQueryParams = FCollisionObjectQueryParams();
	
	MutantObjectQueryParams.AddObjectTypesToQuery(ECC_MESH_TEAM1);
	MutantObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	MutantObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
}

void AMutantCharacter::PerformHandTrace(const TArray<FName>& TraceSockets, TMap<FName, FVector>& PreviousSocketLocations, bool bIsRightHand)
{
	if (!GetMesh()) return;

	for (const FName& SocketName : TraceSockets)
	{
		FVector CurrentLocation = GetMesh()->GetSocketLocation(SocketName);
		
		if (PreviousSocketLocations.Contains(SocketName))
		{
			FVector PreviousLocation = PreviousSocketLocations[SocketName];
			FVector TraceDirection = (CurrentLocation - PreviousLocation).GetSafeNormal();

			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.bTraceComplex = true;
			QueryParams.bReturnPhysicalMaterial = true; // 开启物理材质检测

			float TraceRadius = bIsRightHand ? TraceRadius_R : TraceRadius_L;
			
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceRadius);

			bool bHit = GetWorld()->SweepSingleByObjectType(
				HitResult,
				PreviousLocation,
				CurrentLocation,
				FQuat::Identity,
				MutantObjectQueryParams,
				SphereShape,
				QueryParams
			);
			
			// 绘制胶囊体来可视化扫掠轨迹
			// FVector TraceVector = CurrentLocation - PreviousLocation;
			// float TraceDistance = TraceVector.Size();
			// if (TraceDistance > UE_KINDA_SMALL_NUMBER)
			// {
			//    FVector Center = PreviousLocation + TraceVector * 0.5f;
			//    float HalfHeight = (TraceDistance * 0.5f) + TraceRadius;
			//    FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, TraceVector.GetSafeNormal());
			//    DrawDebugCapsule(GetWorld(), Center, HalfHeight, TraceRadius, CapsuleRot, FColor::Red, false, 10.f, 0, 1.f);
			// }
			
			if (bHit)
			{
				ProcessMutantHit(HitResult, TraceDirection, bIsRightHand);
			}
		}
		
		PreviousSocketLocations.Add(SocketName, CurrentLocation);
	}
}

void AMutantCharacter::ProcessMutantHit(const FHitResult& HitResult, const FVector& TraceDirection, bool bIsRightHand)
{
	AActor* OtherActor = HitResult.GetActor();
	UPrimitiveComponent* OtherComp = HitResult.GetComponent();

	// 防止重复检测
	TArray<AActor*>& TargetHitEnemies = bIsRightHand ? RightHandHitEnemies : LeftHandHitEnemies;
	if (TargetHitEnemies.Contains(OtherActor)) return;
	
	TargetHitEnemies.Add(OtherActor);

	float Damage = bIsLightAttack ? LightAttackDamage : HeavyAttackDamage;

	if (OtherActor->ActorHasTag(TAG_CHARACTER_HUMAN))
	{
		DropBlood(GetMesh(), OtherActor, OtherComp, Damage, HitResult);
	}
	else if (OtherComp && (OtherComp->GetCollisionObjectType() == ECC_WorldStatic || OtherComp->GetCollisionObjectType() == ECC_WorldDynamic))
	{
		// 处理击中墙体/静态网格体的贴花与音效，区分左右手确保单次挥击只触发一次击墙效果
		if (bIsRightHand && !bRightHandHasHitWall)
		{
			bRightHandHasHitWall = true;
			SpawnHitWallEffects(HitResult, TraceDirection);
		}
		else if (!bIsRightHand && !bLeftHandHasHitWall)
		{
			bLeftHandHasHitWall = true;
			SpawnHitWallEffects(HitResult, TraceDirection);
		}
	}
	
	if (IsLocallyControlled())
	{
		ServerApplyDamage(OtherActor, Damage);
	}
}

void AMutantCharacter::SpawnHitWallEffects(const FHitResult& HitResult, const FVector& TraceDirection)
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CharacterAsset)
	{
		// 生成墙壁刀痕贴花
		if (AssetSubsystem->CharacterAsset->WallMarkDecal)
		{
			FVector ProjectionDirection = -HitResult.ImpactNormal;
			FVector SurfaceTangent = FVector::VectorPlaneProject(TraceDirection, HitResult.ImpactNormal).GetSafeNormal();

			FRotator DecalRotation = FRotationMatrix::MakeFromXY(ProjectionDirection, SurfaceTangent).Rotator();

			UGameplayStatics::SpawnDecalAttached(
				AssetSubsystem->CharacterAsset->WallMarkDecal,
				FVector(10.f, 20.f, 20.f), 
				HitResult.GetComponent(),
				NAME_None,
				HitResult.ImpactPoint,
				DecalRotation,
				EAttachLocation::KeepWorldPosition,
				10.f 
			);
		}

		// 根据物理材质播放对应的击打音效
		UMetaSoundSource* HitSound = nullptr;
		switch (UGameplayStatics::GetSurfaceType(HitResult))
		{
		case EPhysicalSurface::SurfaceType1:
			HitSound = AssetSubsystem->CharacterAsset->HitWall_Concrete;
			break;
		case EPhysicalSurface::SurfaceType2:
			HitSound = AssetSubsystem->CharacterAsset->HitWall_Dirt;
			break;
		case EPhysicalSurface::SurfaceType3:
			HitSound = AssetSubsystem->CharacterAsset->HitWall_Metal;
			break;
		case EPhysicalSurface::SurfaceType4:
			HitSound = AssetSubsystem->CharacterAsset->HitWall_Wood;
			break;
		}

		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitResult.ImpactPoint);
		}
	}
}

void AMutantCharacter::ServerApplyDamage_Implementation(AActor* OtherActor, float Damage)
{
	if (bIsDead || OtherActor == nullptr) return;
	
	if (MutationMode == nullptr) MutationMode = GetWorld()->GetAuthGameMode<AMutationMode>();
	if (MutationMode)
	{
		if (MutationMode->GetMatchState() != MatchState::InProgress)
		{
			return;
		}
	}

	FVector HitDirection = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FHitResult FakeHit;
	FakeHit.ImpactPoint = OtherActor->GetActorLocation();

	UGameplayStatics::ApplyPointDamage(
		OtherActor, 
		Damage, 
		HitDirection, 
		FakeHit, 
		Controller, 
		this, 
		UDamageTypeMutantDamage::StaticClass()
	);

	// 是人类触发感染
	if (AHumanCharacter* DamagedCharacter = Cast<AHumanCharacter>(OtherActor))
	{
		if (!DamagedCharacter->bIsDead && !DamagedCharacter->bIsImmune)
		{
			ABaseController* DamagedController = Cast<ABaseController>(DamagedCharacter->GetController());
			if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
			if (MutationMode)
			{
				MutationMode->GetInfect(DamagedCharacter, DamagedController, this, BaseController);
			}
		}
	}
}

void AMutantCharacter::DropBlood(USkeletalMeshComponent* MeshComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, float Damage, const FHitResult& TraceHitResult)
{
	ABaseCharacter* OverlappedCharacter = Cast<ABaseCharacter>(OtherActor);

	if (OverlappedCharacter == nullptr || OverlappedCharacter->BloodEffect == nullptr) return;

	auto BloodEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		OverlappedCharacter->BloodEffect,
		TraceHitResult.ImpactPoint,
		TraceHitResult.ImpactNormal.Rotation()
	);
	if (BloodEffectComponent)
	{
		BloodEffectComponent->SetVariableInt(TEXT("Count"), ULibraryCommon::GetBloodParticleCount(Damage));
		BloodEffectComponent->SetVariableLinearColor(TEXT("Color"), OverlappedCharacter->BloodColor);
	}
	
	auto BloodSmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		OverlappedCharacter->BloodSmokeEffect,
		TraceHitResult.ImpactPoint,
		TraceHitResult.ImpactNormal.Rotation()
	);
	if (BloodSmokeEffectComponent)
	{
		BloodSmokeEffectComponent->SetVariableLinearColor(TEXT("SmokeColor"), OverlappedCharacter->BloodColor);
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

void AMutantCharacter::ApplySuppressionForce(FVector HitDirection, float PushStrength)
{
    if (!HasAuthority() || !GetCharacterMovement()) return;

    if (GetCharacterMovement()->MovementMode == MOVE_Custom && 
        GetCharacterMovement()->CustomMovementMode == CMOVE_Ladder)
    {
        return; 
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastRepelTime < RepelCooldown) return; 
    LastRepelTime = CurrentTime;

    float BaseDuration = 0.16f;
    ExecuteRootMotionPush(HitDirection, PushStrength, BaseDuration);

    if (!IsLocallyControlled())
    {
        Client_PredictSuppressionForce(HitDirection, PushStrength);
    }
}

void AMutantCharacter::Client_PredictSuppressionForce_Implementation(FVector HitDirection, float PushStrength)
{
    LastRepelTime = GetWorld()->GetTimeSeconds();
    
    // 计算单程网络延迟 (Ping)
    float PingSeconds = 0.05f;
    if (APlayerState* PS = GetPlayerState())
    {
        PingSeconds = PS->GetPingInMilliseconds() / 1000.f;
    }

    // 将原本的 0.2 秒扣除掉延迟时间
    float AdjustedDuration = FMath::Max(0.01f, 0.2f - PingSeconds); // 使用AdjustedDuration避免客户端本地被击退又拉回

    ExecuteRootMotionPush(HitDirection, PushStrength, AdjustedDuration);
}

void AMutantCharacter::ExecuteRootMotionPush(FVector HitDirection, float PushStrength, float Duration)
{
    FVector PushDir = HitDirection;
    if (!PushDir.Normalize()) return;

    TSharedPtr<FRootMotionSource_ConstantForce> SmoothPush = MakeShared<FRootMotionSource_ConstantForce>();
	
    SmoothPush->InstanceName = FName("HitSuppression");
    SmoothPush->Force = PushDir * PushStrength; 
    SmoothPush->Duration = Duration; // 使用传进来的动态时间
    SmoothPush->AccumulateMode = ERootMotionAccumulateMode::Additive;
    SmoothPush->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	
    GetCharacterMovement()->ApplyRootMotionSource(SmoothPush);
}

bool AMutantCharacter::CanInteract(ABaseCharacter* Interactor)
{
	if (!bIsDead || bSuckedDry || !bKilledByMelee) return false;
	
	if (!Interactor || Interactor->bIsDead)
	{
		return false;
	}

	if (AHumanCharacter* Human = Cast<AHumanCharacter>(Interactor))
	{
		if (Human->bIsImmune) return false;
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
