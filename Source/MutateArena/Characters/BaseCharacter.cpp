#include "BaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "..\PlayerStates\TeamType.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "CommonInputSubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/Equipments/Data/DamageTypeFall.h"
#include "MutateArena/Equipments/Projectiles/ProjectileBullet.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/PlayerSubsystem.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/OverheadWidget.h"
#include "Components/WidgetComponent.h"
#include "Data/CharacterAsset.h"
#include "GameFramework/SpringArmComponent.h"
#include "Data/InputAsset.h"
#include "Interfaces/InteractableTarget.h"
#include "MutateArena/Effects/BloodCollision.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/System/Data/CommonAsset.h"
#include "MutateArena/UI/TextChat/TextChat.h"
#include "Net/UnrealNetwork.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh(), SOCKET_CAMERA);
	CameraBoom->TargetArmLength = 0.f;
	// CameraBoom->bEnableCameraLag = true;
	// CameraBoom->CameraLagSpeed = 15.f;
	// CameraBoom->bEnableCameraRotationLag = true;
	// CameraBoom->CameraRotationLagSpeed = 15.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(Camera);
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetMesh(), SOCKET_CAMERA);
	OverheadWidget->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	OverheadWidget->SetDrawSize(FVector2D(120.f, 30.f));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(50.f);
	GetCharacterMovement()->AirControl = 0.4;
	GetCharacterMovement()->AirControlBoostMultiplier = 1;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	Tags.Add(TAG_CHARACTER_BASE);
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ControllerPitch);
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 监听输入设备类型改变
	if (UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		if (!CommonInputSubsystem->OnInputMethodChangedNative.IsBoundToObject(this))
		{
			CommonInputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::OnInputMethodChanged);
		}
	}

	if (OverheadWidget)
	{
		if (OverheadWidgetClass == nullptr) OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject());
		if (OverheadWidgetClass)
		{
			OverheadWidgetClass->BaseCharacter = this;
		}
	}

	GetMesh()->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnHit);
}

// 增强输入
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->InputAsset == nullptr) return;

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// 观察者销毁时不触发ABaseController::OnUnPossess，在这里移除观察者绑定的输入映射
			Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->BaseMappingContext);
			Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->SpectatorMappingContext);

			Subsystem->AddMappingContext(AssetSubsystem->InputAsset->BaseMappingContext, 100);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Started, this, &ThisClass::MoveStarted);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Completed, this, &ThisClass::MoveCompleted);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::LookMouse);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->LookStickAction, ETriggerEvent::Triggered, this, &ThisClass::LookStick);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->JumpAction, ETriggerEvent::Triggered, this, &ThisClass::JumpButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->CrouchAction, ETriggerEvent::Started, this, &ThisClass::CrouchButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->CrouchAction, ETriggerEvent::Completed, this, &ThisClass::CrouchButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->CrouchControllerAction, ETriggerEvent::Triggered, this, &ThisClass::CrouchControllerButtonPressed);
		
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Started, this, &ThisClass::InteractStarted);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Ongoing, this, &ThisClass::InteractOngoing);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Triggered, this, &ThisClass::InteractTriggered);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Completed, this, &ThisClass::InteractCompleted);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Canceled, this, &ThisClass::InteractCanceled);

		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->ScoreboardAction, ETriggerEvent::Triggered, this, &ThisClass::ScoreboardButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->ScoreboardAction, ETriggerEvent::Completed, this, &ThisClass::ScoreboardButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->PauseMenuAction, ETriggerEvent::Triggered, this, &ThisClass::PauseMenuButtonPressed);

		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->RadialMenuAction, ETriggerEvent::Triggered, this, &ThisClass::RadialMenuButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->RadialMenuAction, ETriggerEvent::Completed, this, &ThisClass::RadialMenuButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->RadialMenuChangeAction, ETriggerEvent::Triggered, this, &ThisClass::RadialMenuChangeButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->RadialMenuSelectAction, ETriggerEvent::Triggered, this, &ThisClass::RadialMenuSelect);

		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->TextChatAction, ETriggerEvent::Triggered, this, &ThisClass::TextChat);
	}
}

void ABaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PollInit_PlayerStateTeam();

	PollInit_ControllerAndPSAndTeam();

	CalcAimPitch();
}

void ABaseCharacter::PollInit_PlayerStateTeam()
{
	if (!bIsPlayerStateTeamReady)
	{
		// 设置碰撞
		if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
		if (BasePlayerState && BasePlayerState->Team != ETeam::NoTeam)
		{
			bIsPlayerStateTeamReady = true;

			switch (BasePlayerState->Team)
			{
			case ETeam::Team1:
				GetMesh()->SetCollisionObjectType(ECC_MESH_TEAM1);
				break;
			case ETeam::Team2:
				GetMesh()->SetCollisionObjectType(ECC_MESH_TEAM2);
				break;
			}
		}
	}
}

void ABaseCharacter::PollInit_ControllerAndPSAndTeam()
{
	if (IsLocallyControlled() && !bIsControllerAndPSAndTeamReady)
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
		if (BaseController)
		{
			if (!bIsLocallyControllerReady)
			{
				bIsLocallyControllerReady = true;
				
				OnLocallyControllerReady();
			}
			
			// InitHUD依赖Controller和Controller中的PlayerState和Controller中的PlayerState中的Team
			ABasePlayerState* PS = Cast<ABasePlayerState>(BaseController->PlayerState);
			if (PS && PS->Team != ETeam::NoTeam)
			{
				bIsControllerAndPSAndTeamReady = true;

				BaseController->InitHUD();
			}
		}
	}
}

void ABaseCharacter::OnLocallyControllerReady()
{
}

// 计算俯仰
void ABaseCharacter::CalcAimPitch()
{
	// 服务端把ControllerPitch复制到客户端
	if (HasAuthority())
	{
		ControllerPitch = MappingAimPitch(GetViewRotation().Pitch);
		AimPitch = ControllerPitch;
	}
	else
	{
		if (IsLocallyControlled())
		{
			AimPitch = MappingAimPitch(GetViewRotation().Pitch);
		}
		// 非本地使用服务端复制下来的ControllerPitch
		else
		{
			AimPitch = ControllerPitch;
		}
	}
}

float ABaseCharacter::MappingAimPitch(float TempAimPitch)
{
	if (TempAimPitch > 90.f)
	{
		FVector2D InRange(360.f, 270.f);
		FVector2D OutRange(0.f, -90.f);
		TempAimPitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, TempAimPitch);
	}

	return TempAimPitch;
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 初始化ASC
	InitAbilityActorInfo();

	OnAbilitySystemComponentInit();

	if (AbilitySystemComponent)
	{
		// 赋予默认值Effect
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttrEffect, GetCharacterLevel(), EffectContext);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
		}
	}
}

void ABaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();

	OnAbilitySystemComponentInit();
}

void ABaseCharacter::Destroyed()
{
	if (IsLocallyControlled())
	{
		if (UPlayerSubsystem* PlayerSubsystem = ULocalPlayer::GetSubsystem<UPlayerSubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
		{
			PlayerSubsystem->SetIsDead();
		}
	}

	Super::Destroyed();
}

// 输入设备类型改变
void ABaseCharacter::OnInputMethodChanged(ECommonInputType TempInputType)
{
	UE_LOG(LogTemp, Warning, TEXT("OnInputMethodChanged: %d"), TempInputType);

	ServerSetInputType(TempInputType);
}

void ABaseCharacter::ServerSetInputType_Implementation(ECommonInputType TempInputType)
{
	if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
	if (BasePlayerState)
	{
		BasePlayerState->InputType = TempInputType;
	}
}

void ABaseCharacter::InitAbilityActorInfo()
{
	if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
	if (BasePlayerState)
	{
		AbilitySystemComponent = Cast<UMAAbilitySystemComponent>(BasePlayerState->GetAbilitySystemComponent());
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->InitAbilityActorInfo(BasePlayerState, this);
		}
		AttributeSetBase = BasePlayerState->GetAttributeSetBase();
	}
}

void ABaseCharacter::OnAbilitySystemComponentInit()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnMaxHealthChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	}
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSetBase* ABaseCharacter::GetAttributeSetBase()
{
	return AttributeSetBase;
}

float ABaseCharacter::GetMaxHealth()
{
	return AttributeSetBase ? AttributeSetBase->GetMaxHealth() : 0.f;
}

float ABaseCharacter::GetHealth()
{
	return AttributeSetBase ? AttributeSetBase->GetHealth() : 0.f;
}

float ABaseCharacter::GetDamageReceivedMul()
{
	return AttributeSetBase ? AttributeSetBase->GetDamageReceivedMul() : 0.f;
}

float ABaseCharacter::GetRepelReceivedMul()
{
	return AttributeSetBase ? AttributeSetBase->GetRepelReceivedMul() : 0.f;
}

float ABaseCharacter::GetCharacterLevel()
{
	return AttributeSetBase ? AttributeSetBase->GetCharacterLevel() : 0.f;
}

float ABaseCharacter::GetMaxWalkSpeed()
{
	return AttributeSetBase ? AttributeSetBase->GetMaxWalkSpeed() : 0.f;
}

float ABaseCharacter::GetJumpZVelocity()
{
	return AttributeSetBase ? AttributeSetBase->GetJumpZVelocity() : 0.f;
}

void ABaseCharacter::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (BloodEffect)
	{
		// UE_LOG(LogTemp, Warning, TEXT("ABaseCharacter::OnHit Location: %s"), *Hit.ImpactPoint.ToString());
		// UE_LOG(LogTemp, Warning, TEXT("ABaseCharacter::OnHit Rotation: %s"), *Hit.ImpactNormal.Rotation().ToString());

		FRotator HitRotation = Hit.ImpactNormal.Rotation();
		auto BloodEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			BloodEffect,
			Hit.ImpactPoint,
			FRotator(-HitRotation.Pitch, HitRotation.Yaw + 180.f, HitRotation.Roll)
		);
		if (BloodEffectComponent)
		{
			if (AProjectileBullet* ProjectileBullet = Cast<AProjectileBullet>(OtherActor))
			{
				float Damage = ProjectileBullet->GetDamage(Hit.Distance);
				BloodEffectComponent->SetVariableInt(TEXT("Count"), ULibraryCommon::GetBloodParticleCount(Damage));
			}
			BloodEffectComponent->SetVariableLinearColor(TEXT("Color"), BloodColor);

			UBloodCollision* CollisionCB = NewObject<UBloodCollision>(this);
			BloodEffectComponent->SetVariableObject(TEXT("CollisionCB"), CollisionCB);
		}
	}
}

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	FVector2D AxisVector = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), AxisVector.Y);
	AddMovementInput(GetActorRightVector(), AxisVector.X);
}

// 分开处理Look，无需根据输入设备类型区分灵敏度，支持同时使用键鼠和手柄控制一个角色
void ABaseCharacter::LookMouse(const FInputActionValue& Value)
{
	FVector2D AxisVector = Value.Get<FVector2D>();
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	AddControllerYawInput(AxisVector.X * StorageSubsystem->CacheSetting->MouseSensitivity);
	AddControllerPitchInput(AxisVector.Y * StorageSubsystem->CacheSetting->MouseSensitivity);
}

void ABaseCharacter::LookStick(const FInputActionValue& Value)
{
	FVector2D AxisVector = Value.Get<FVector2D>();
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	AddControllerYawInput(AxisVector.X * StorageSubsystem->CacheSetting->ControllerSensitivity);
	AddControllerPitchInput(AxisVector.Y * StorageSubsystem->CacheSetting->ControllerSensitivity);
}

void ABaseCharacter::JumpButtonPressed(const FInputActionValue& Value)
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Jump();
	}
}

// 键鼠为长按蹲
void ABaseCharacter::CrouchButtonPressed(const FInputActionValue& Value)
{
	if (!GetCharacterMovement()->IsFalling())
	{
		Crouch();
	}
}

void ABaseCharacter::CrouchButtonReleased(const FInputActionValue& Value)
{
	if (!GetCharacterMovement()->IsFalling())
	{
		UnCrouch();
	}
}

// 手柄为切换蹲
void ABaseCharacter::CrouchControllerButtonPressed(const FInputActionValue& Value)
{
	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABaseCharacter::TraceInteractTarget(FHitResult& OutHit)
{
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * 160.f;

	// DrawDebugLine(GetWorld(), Start, End, C_YELLOW, true);

	FCollisionQueryParams QueryParams;
	TArray<AActor*> TeamPlayers;
	
	if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState)
	{
		QueryParams.AddIgnoredActors(BaseGameState->AllEquipments);

		TArray<ABasePlayerState*> PlayerStates = BaseGameState->GetPlayerStates({});
		for (int32 i = 0; i < PlayerStates.Num(); ++i)
		{
			if (PlayerStates[i] && PlayerStates[i]->GetHealth() > 0.f)
			{
				TeamPlayers.AddUnique(PlayerStates[i]->GetPawn());
			}
		}
	}
	QueryParams.AddIgnoredActors(TeamPlayers);
	
	GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		ECollisionChannel::ECC_Visibility,
		FCollisionShape::MakeSphere(10.f),
		QueryParams
	);
}

void ABaseCharacter::InteractStarted(const FInputActionValue& Value)
{
	FHitResult OutHit;
	TraceInteractTarget(OutHit);
	
	if (OutHit.bBlockingHit)
	{
		if (IInteractableTarget* Target = Cast<IInteractableTarget>(OutHit.GetActor()))
		{
			if (Target->CanInteract())
			{
				InteractTarget = OutHit.GetActor();

				if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
				if (BaseController)
				{
					BaseController->OnInteractStarted.Broadcast();
				}
				
				return;
			}
		}
	}

	InteractTarget = nullptr;
}

void ABaseCharacter::InteractOngoing(const FInputActionValue& Value)
{
	if (InteractTarget != nullptr)
	{
		FHitResult OutHit;
		TraceInteractTarget(OutHit);
		if (OutHit.bBlockingHit)
		{
			if (InteractTarget == OutHit.GetActor())
			{
				if (IInteractableTarget* Target = Cast<IInteractableTarget>(OutHit.GetActor()))
				{
					if (Target->CanInteract())
					{
						return;
					}
				}
			}
		}
	}

	// 停止交互
	InteractTarget = nullptr;
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->OnInteractEnded.Broadcast();
	}
}

void ABaseCharacter::InteractTriggered(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController && InteractTarget)
	{
		if (IInteractableTarget* Target = Cast<IInteractableTarget>(InteractTarget))
		{
			Target->OnInteract(this);

			ServerInteractTriggered(InteractTarget);
		}
	}
}

// 在服务端通知交互目标被交互了，以便复制到所有客户端。
void ABaseCharacter::ServerInteractTriggered_Implementation(AActor* TempInteractTarget)
{
	if (IInteractableTarget* Target = Cast<IInteractableTarget>(TempInteractTarget))
	{
		Target->OnInteract_Server();
	}
}

void ABaseCharacter::InteractCompleted(const FInputActionValue& Value)
{
	InteractTarget = nullptr;

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->OnInteractEnded.Broadcast();
	}
}

void ABaseCharacter::InteractCanceled(const FInputActionValue& Value)
{
	InteractTarget = nullptr;

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->OnInteractEnded.Broadcast();
	}
}

void ABaseCharacter::ScoreboardButtonPressed(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowScoreboard.Broadcast(true);
	}
}

void ABaseCharacter::ScoreboardButtonReleased(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowScoreboard.Broadcast(false);
	}
}

void ABaseCharacter::PauseMenuButtonPressed(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowPauseMenu.Broadcast();
	}
}

void ABaseCharacter::RadialMenuButtonPressed(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowRadialMenu.Broadcast(true);
	}
}

void ABaseCharacter::RadialMenuButtonReleased(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowRadialMenu.Broadcast(false);
	}
}

void ABaseCharacter::RadialMenuChangeButtonPressed(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ChangeRadialMenu.Broadcast();
	}
}

// TODO 弦操作使用IA_RadialMenu时，手柄RadialMenuButtonPressed不触发，使用了新建的IA_RadialMenuSelectChord
void ABaseCharacter::RadialMenuSelect(const FInputActionValue& Value)
{
	FVector2D AxisVector = Value.Get<FVector2D>();
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->SelectRadialMenu.Broadcast(AxisVector.X, AxisVector.Y);
	}
}

void ABaseCharacter::TextChat(const FInputActionValue& Value)
{
	// TODO 手柄暂未处理
	if (UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		if (CommonInputSubsystem->GetCurrentInputType() != ECommonInputType::MouseAndKeyboard)
		{
			return;
		}
	}

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->FocusUI();
		BaseController->ShowTextChat.Broadcast();
	}
}

void ABaseCharacter::SetHealth(float TempHealth)
{
	if (AttributeSetBase)
	{
		AttributeSetBase->SetHealth(TempHealth);
	}
}

// 使用RPC通知攻击者立刻响应受伤者血量变化（UAttributeSetBase中的Health同步有点慢）
void ABaseCharacter::MulticastSetHealth_Implementation(float TempHealth, AController* AttackerController)
{
	if (HasAuthority()) return;

	if (AttackerController && AttackerController->IsLocalController())
	{
		SetHealth(TempHealth);
	}
}

void ABaseCharacter::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	if (OverheadWidgetClass == nullptr) OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject());
	if (OverheadWidgetClass)
	{
		OverheadWidgetClass->OnMaxHealthChange(Data.NewValue);
	}

	if (IsLocallyControlled())
	{
		// 低血量屏幕变灰
		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->CharacterAsset && AssetSubsystem->CharacterAsset->MPC_LowHealth)
		{
			if (Data.OldValue == 0.f || Data.NewValue == 0.f) return;
			float OldRate = Data.OldValue / GetMaxHealth();
			float NewRate = Data.NewValue / GetMaxHealth();
			
			// 没有跨过阈值
			if (OldRate > HealthRateThreshold && NewRate > HealthRateThreshold || OldRate < HealthRateThreshold && NewRate < HealthRateThreshold) return;
			if (UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(AssetSubsystem->CharacterAsset->MPC_LowHealth))
			{
				float TempDesaturation = NewRate < HealthRateThreshold ? Desaturation : 0.f;
				MPCI->SetScalarParameterValue(FName("Desaturation"), TempDesaturation);
				MPCI->SetScalarParameterValue(FName("TriggerTime"), GetWorld()->GetTimeSeconds());
			}
		}
	}
}

void ABaseCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (OverheadWidgetClass == nullptr) OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject());
	if (OverheadWidgetClass)
	{
		OverheadWidgetClass->OnHealthChange(Data.OldValue, Data.NewValue);
	}

	if (IsLocallyControlled())
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
		if (BaseController)
		{
			BaseController->SetHUDHealth(Data.NewValue);
		}

		// 低血量屏幕变灰
		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->CharacterAsset && AssetSubsystem->CharacterAsset->MPC_LowHealth)
		{
			if (GetMaxHealth() == 0.f) return;
			float OldRate = Data.OldValue / GetMaxHealth();
			float NewRate = Data.NewValue / GetMaxHealth();
			
			// 没有跨过阈值
			if (OldRate > HealthRateThreshold && NewRate > HealthRateThreshold || OldRate < HealthRateThreshold && NewRate < HealthRateThreshold) return;
			if (UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(AssetSubsystem->CharacterAsset->MPC_LowHealth))
			{
				float TempDesaturation = NewRate < HealthRateThreshold ? Desaturation : 0.f;
				MPCI->SetScalarParameterValue(FName("Desaturation"), TempDesaturation);
				MPCI->SetScalarParameterValue(FName("TriggerTime"), GetWorld()->GetTimeSeconds());
			}
		}
	}
}

// 落地事件（只在本地和服务端执行）
void ABaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (HasAuthority())
	{
		// 计算扣血倍率
		float DamageRate = CalcFallDamageRate();
		if (DamageRate == 0.f) return;

		// 播放叫声
		MulticastPlayOuchSound(DamageRate);

		// 应用伤害
		UGameplayStatics::ApplyDamage(this, GetMaxHealth() * DamageRate, Controller, this, UDamageTypeFall::StaticClass());
	}
}

// 计算跌落伤害比例
float ABaseCharacter::CalcFallDamageRate()
{
	FVector Velocity = GetCharacterMovement()->Velocity; // Landed判定的时机是即将落地时，此时速度达到最大
	float Gravity = GetCharacterMovement()->GetGravityZ();
	float DiffHighMeter = Velocity.Z / Gravity;

	// 角色降落时不遵循自由落体，大致模拟跌落伤害
	float DamageRate; // 跌落扣血比例（占MaxHealth）
	if (DiffHighMeter < 1.f) // 大约对应游戏里5m
	{
		DamageRate = 0.f;
	}
	else if (DiffHighMeter >= 1.f && DiffHighMeter < 1.2f)
	{
		DamageRate = 0.05f;
	}
	else if (DiffHighMeter >= 1.2f && DiffHighMeter < 1.5f)
	{
		DamageRate = 0.1f;
	}
	else
	{
		DamageRate = 0.15f;
	}

	return DamageRate;
}

void ABaseCharacter::MulticastPlayOuchSound_Implementation(float DamageRate)
{
	UGameplayStatics::PlaySoundAtLocation(this, OuchSound, GetActorLocation());
}

// 根据地形播放不同脚步声
void ABaseCharacter::PlayFootSound()
{
	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 100.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params);
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (HitResult.bBlockingHit && AssetSubsystem && AssetSubsystem->CharacterAsset)
	{
		UMetaSoundSource* Sound = AssetSubsystem->CharacterAsset->FootSound_Concrete;
		switch (UGameplayStatics::GetSurfaceType(HitResult))
		{
		case EPhysicalSurface::SurfaceType1:
			Sound = AssetSubsystem->CharacterAsset->FootSound_Concrete;
			break;
		case EPhysicalSurface::SurfaceType2:
			Sound = AssetSubsystem->CharacterAsset->FootSound_Dirt;
			break;
		case EPhysicalSurface::SurfaceType3:
			Sound = AssetSubsystem->CharacterAsset->FootSound_Metal;
			break;
		case EPhysicalSurface::SurfaceType4:
			Sound = AssetSubsystem->CharacterAsset->FootSound_Wood;
			break;
		}
		UGameplayStatics::PlaySoundAtLocation(this, Sound, HitResult.Location);
	}
}

void ABaseCharacter::PlayFootLandSound()
{
	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 100.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params);
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (HitResult.bBlockingHit && AssetSubsystem && AssetSubsystem->CharacterAsset)
	{
		UMetaSoundSource* Sound = AssetSubsystem->CharacterAsset->FootLandSound_Concrete;
		switch (UGameplayStatics::GetSurfaceType(HitResult))
		{
		case EPhysicalSurface::SurfaceType1:
			Sound = AssetSubsystem->CharacterAsset->FootLandSound_Concrete;
			break;
		case EPhysicalSurface::SurfaceType2:
			Sound = AssetSubsystem->CharacterAsset->FootLandSound_Dirt;
			break;
		case EPhysicalSurface::SurfaceType3:
			Sound = AssetSubsystem->CharacterAsset->FootLandSound_Metal;
			break;
		case EPhysicalSurface::SurfaceType4:
			Sound = AssetSubsystem->CharacterAsset->FootLandSound_Wood;
			break;
		}
		UGameplayStatics::PlaySoundAtLocation(this, Sound, HitResult.Location);
	}
}

void ABaseCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	UGameplayStatics::ApplyDamage(this, 999999, BaseController, this, UDamageTypeFall::StaticClass());

	// Super::FellOutOfWorld(DmgType);
}

void ABaseCharacter::SendRadio(int32 RadioIndex)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (BaseController && BasePlayerState && AssetSubsystem&& AssetSubsystem->CharacterAsset)
	{
		BaseController->ServerSendMsg(
			EMsgType::Radio,
			BasePlayerState->Team,
			BasePlayerState->GetPlayerName(),
			AssetSubsystem->CharacterAsset->RadioTexts[RadioIndex]
		);
	}

	LocalPlayRadioSound(RadioIndex);
	ServerPlayRadioSound(RadioIndex);
}

void ABaseCharacter::ServerPlayRadioSound_Implementation(int32 RadioIndex)
{
	MulticastPlayRadioSound(RadioIndex);
}

void ABaseCharacter::MulticastPlayRadioSound_Implementation(int32 RadioIndex)
{
	if (IsLocallyControlled())
	{
		LocalPlayRadioSound(RadioIndex);
	}
}

void ABaseCharacter::LocalPlayRadioSound(int32 RadioIndex)
{
	if (RadioSounds.IsValidIndex(RadioIndex))
	{
		UGameplayStatics::PlaySoundAtLocation(this, RadioSounds[RadioIndex], GetActorLocation());
	}
}

void ABaseCharacter::SprayPaint(int32 RadioIndex)
{
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * 300.f;

	DrawDebugLine(GetWorld(), Start, End, C_YELLOW, true);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	TArray<AActor*> AllEquipments;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TAG_EQUIPMENT, AllEquipments);
	QueryParams.AddIgnoredActors(AllEquipments);

	FHitResult OutHit;
	GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		ECollisionChannel::ECC_Visibility,
		FCollisionShape::MakeSphere(5.f),
		QueryParams
	);
	
	if (OutHit.bBlockingHit)
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->CommonAsset)
		{
			auto SprayPaints = AssetSubsystem->CommonAsset->SprayPaints;
			if (SprayPaints.IsValidIndex(RadioIndex))
			{
				if (IsValid(SprayPaintDecal))
				{
					SprayPaintDecal->DestroyComponent();
				}
				
				FRotator DecalRotation = FRotationMatrix::MakeFromX(OutHit.ImpactNormal).Rotator();
				DecalRotation.Roll += 90.f;
				DecalRotation.Yaw += 180.f;
				SprayPaintDecal = UGameplayStatics::SpawnDecalAttached(
					SprayPaints[RadioIndex].Material,
					FVector(5.f, 100.f, 100.f),
					OutHit.GetComponent(),
					OutHit.BoneName,
					OutHit.ImpactPoint,
					DecalRotation,
					EAttachLocation::KeepWorldPosition,
					0.f
				);
			}
		}
	}
}
