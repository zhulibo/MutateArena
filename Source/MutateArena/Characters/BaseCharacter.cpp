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
#include "Components/OverheadWidget.h"
#include "Components/WidgetComponent.h"
#include "Data/CharacterAsset.h"
#include "GameFramework/SpringArmComponent.h"
#include "Data/InputAsset.h"
#include "Interfaces/InteractableTarget.h"
#include "MutateArena/Effects/BloodCollision.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Net/UnrealNetwork.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh(), TEXT("CameraSocket"));
	CameraBoom->TargetArmLength = 0.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(Camera);
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetMesh(), TEXT("CameraSocket"));
	OverheadWidget->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); // TODO 垂直向上

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

	PollSetMeshCollision();

	CalcAimPitch();

	PollInit();
}

// 设置碰撞
void ABaseCharacter::PollSetMeshCollision()
{
	if (!HasInitMeshCollision)
	{
		if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
		if (BasePlayerState && BasePlayerState->GetTeam() != ETeam::NoTeam)
		{
			switch (BasePlayerState->GetTeam())
			{
			case ETeam::Team1:
				GetMesh()->SetCollisionObjectType(ECC_TEAM1_MESH);
				break;
			case ETeam::Team2:
				GetMesh()->SetCollisionObjectType(ECC_TEAM2_MESH);
				break;
			}

			HasInitMeshCollision = true;
		}
	}
}

void ABaseCharacter::OnControllerReady()
{
	// 闪光弹
	if (IsLocallyControlled())
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->CharacterAsset)
		{
			if (SceneCapture)
			{
				SceneCapture->TextureTarget = AssetSubsystem->CharacterAsset->RT_Flashbang;
			}

			FlashbangMID = UMaterialInstanceDynamic::Create(AssetSubsystem->CharacterAsset->MI_Flashbang, this);
			if (FlashbangMID)
			{
				Camera->PostProcessSettings.AddBlendable(FlashbangMID, 1.f);
			}
		}
	}
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

void ABaseCharacter::PollInit()
{
	if (IsLocallyControlled() && !bIsControllerReady)
	{
		BaseController = Cast<ABaseController>(Controller);
		if (BaseController)
		{
			bIsControllerReady = true;

			OnControllerReady();

			BaseController->ManualReset();
		}
	}
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
void ABaseCharacter::OnInputMethodChanged(ECommonInputType TempCommonInputType)
{
	UE_LOG(LogTemp, Warning, TEXT("OnInputMethodChanged: %d"), TempCommonInputType);
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
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			AttributeSetBase->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnMaxHealthChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			AttributeSetBase->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
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

float ABaseCharacter::GetJumpZVelocity()
{
	return AttributeSetBase ? AttributeSetBase->GetJumpZVelocity() : 0.f;
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
	UGameplayStatics::ApplyDamage(this, GetHealth(), BaseController, this, UDamageTypeFall::StaticClass());

	// Super::FellOutOfWorld(DmgType);
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
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector Position;
	FVector Direction;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation, Position, Direction
	);

	if (bScreenToWorld)
	{
		FVector Start = Position;
		FVector End = Position + Direction * 160.f;

		DrawDebugLine(GetWorld(), Start, End, C_YELLOW, true);

		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(this);
		GetWorld()->SweepSingleByChannel(
			OutHit,
			Start,
			End,
			FQuat::Identity,
			ECollisionChannel::ECC_Visibility,
			FCollisionShape::MakeSphere(10.f),
			CollisionQueryParams
		);
	}
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
		Target->OnInteractOnServer();
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
	if (OuchSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OuchSound, GetActorLocation());
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
}

void ABaseCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (OverheadWidgetClass == nullptr) OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject());
	if (OverheadWidgetClass)
	{
		OverheadWidgetClass->OnHealthChange(Data.OldValue, Data.NewValue);
	}

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->SetHUDHealth(Data.NewValue);
	}
}
