#include "BaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"
#include "CommonInputSubsystem.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/Equipments/Data/DamageTypeFall.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/PlayerSubsystem.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "Camera/CameraComponent.h"
#include "Components/AutoHostComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/InteractorComponent.h"
#include "Components/MAMovementComponent.h"
#include "Components/OverheadWidget.h"
#include "Components/WidgetComponent.h"
#include "Data/CharacterAsset.h"
#include "GameFramework/SpringArmComponent.h"
#include "Data/InputAsset.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StateTreeComponent.h"
#include "Data/DNAAsset2.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "MutateArena/UI/TextChat/TextChat.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"

#define LOCTEXT_NAMESPACE "ABaseCharacter"

ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMAMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	MovementComp = Cast<UMAMovementComponent>(GetCharacterMovement());
	
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
	OverheadWidget->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	OverheadWidget->SetDrawSize(FVector2D(120.f, 30.f));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(50.f);
	// 空中控制
	GetCharacterMovement()->GravityScale = 1.2f;
	GetCharacterMovement()->AirControl = 0.4f;
	GetCharacterMovement()->AirControlBoostMultiplier = 2.f;
	GetCharacterMovement()->AirControlBoostVelocityThreshold = 40.f;
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	Tags.Add(TAG_CHARACTER_BASE);
	
	InteractorComp = CreateDefaultSubobject<UInteractorComponent>(TEXT("InteractorComponent"));
	
	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
	StateTreeComp->SetStartLogicAutomatically(false);
	
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	AIPerceptionComp->SetAutoActivate(false);
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2000.0f; // 视野半径
	SightConfig->LoseSightRadius = 2200.0f; // 丢失视野半径
	SightConfig->PeripheralVisionAngleDegrees = 90.0f; // 视角(90度，即前方180度范围)
	SightConfig->SetMaxAge(3.0f); // 记忆时间 (目标消失5秒后忘记)
	// 让感知系统能检测所有阵营（因为默认只能检测"敌人"，而PlayerController没有默认的TeamID逻辑，可能需实现队伍接口）
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	StimuliSourceComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSourceComponent"));
	StimuliSourceComp->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSourceComp->RegisterWithPerceptionSystem();
	
	AutoHostComp = CreateDefaultSubobject<UAutoHostComponent>(TEXT("AutoHostComponent"));
	
	// 小地图
	MinimapBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("MinimapBoom"));
	MinimapBoom->SetupAttachment(GetCapsuleComponent()); // 挂载到胶囊体上，避免受角色动画影响
	// MinimapBoom->SetUsingAbsoluteRotation(true);
	MinimapBoom->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // 绝对俯视视角
	MinimapBoom->TargetArmLength = 2000.f; // 小地图相机高度（视你的地图大小调整）
	MinimapBoom->bDoCollisionTest = false; // 必须关闭，防止碰到天花板相机拉近
	MinimapBoom->bInheritPitch = false;
	MinimapBoom->bInheritRoll = false;
	MinimapBoom->bInheritYaw = true;       // 如果设为 true，小地图会随着玩家视角旋转；如果设为 false，小地图固定上北下南

	MinimapCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCapture"));
	MinimapCapture->SetupAttachment(MinimapBoom, USpringArmComponent::SocketName);
	MinimapCapture->ProjectionType = ECameraProjectionMode::Orthographic; // 小地图必须用正交投影
	MinimapCapture->OrthoWidth = GameConstants::MinimapOrthoWidth; // 捕获的范围宽度（视需求调整）
	MinimapCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR; // LDR 颜色直接给 UI 用最好
	MinimapCapture->bCaptureEveryFrame = false;
	MinimapCapture->bCaptureOnMovement = false;
	MinimapCapture->ShowFlags.SetLighting(false);
	MinimapCapture->ShowFlags.SetDynamicShadows(false);
	MinimapCapture->ShowFlags.SetPostProcessing(true);
	MinimapCapture->ShowFlags.SetAtmosphere(false);
	MinimapCapture->ShowFlags.SetFog(false);
	MinimapCapture->ShowFlags.SetParticles(false);
	MinimapCapture->ShowFlags.SetSkeletalMeshes(false);
}

void ABaseCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const FName HeadBoneName = FName("Head");
		if (MeshComp->DoesSocketExist(HeadBoneName))
		{
			OutLocation = MeshComp->GetSocketLocation(HeadBoneName);
			OutRotation = MeshComp->GetSocketRotation(HeadBoneName);
			return;
		}
	}

	Super::GetActorEyesViewPoint(OutLocation, OutRotation);
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ControllerPitch);
	DOREPLIFETIME(ThisClass, bIsDead);
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnLadderBeginOverlap);
		GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnLadderEndOverlap);
	}
	
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

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Started, this, &ThisClass::MoveStarted);
		EIC->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
		EIC->BindAction(AssetSubsystem->InputAsset->MoveAction, ETriggerEvent::Completed, this, &ThisClass::MoveCompleted);
		EIC->BindAction(AssetSubsystem->InputAsset->LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::LookMouse);
		EIC->BindAction(AssetSubsystem->InputAsset->LookStickAction, ETriggerEvent::Triggered, this, &ThisClass::LookStick);
		EIC->BindAction(AssetSubsystem->InputAsset->JumpAction, ETriggerEvent::Triggered, this, &ThisClass::JumpButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->CrouchAction, ETriggerEvent::Started, this, &ThisClass::CrouchButtonPressed);
		EIC->BindAction(AssetSubsystem->InputAsset->CrouchAction, ETriggerEvent::Completed, this, &ThisClass::CrouchButtonReleased);
		EIC->BindAction(AssetSubsystem->InputAsset->CrouchControllerAction, ETriggerEvent::Triggered, this, &ThisClass::CrouchControllerButtonPressed);
		
		EIC->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Started, InteractorComp, &UInteractorComponent::InteractStarted);
		EIC->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Ongoing, InteractorComp, &UInteractorComponent::InteractOngoing);
		EIC->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Triggered, InteractorComp, &UInteractorComponent::InteractTriggered);
		EIC->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Completed, InteractorComp, &UInteractorComponent::InteractCompleted);
		EIC->BindAction(AssetSubsystem->InputAsset->InteractAction, ETriggerEvent::Canceled, InteractorComp, &UInteractorComponent::InteractCanceled);
	}
}

void ABaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PollInit_PlayerStateTeam();
	PollInit_ControllerAndPSAndTeam();
	CalcAimPitch();
	
	if (IsLocallyControlled())
	{
		UpdateHurtEffect(DeltaSeconds);
	}
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
	// 只有本地激活感知
	AIPerceptionComp->Activate(true);
	
	// 检测挂机
	if (AutoHostComp)
	{
		AutoHostComp->StartAFKCheck();
	}
	
	if (MinimapCapture)
	{
		// TODO 每帧开启会导致骨骼网格体开启了Nanite的角色没有动画
		// MinimapCapture->bCaptureEveryFrame = true;
		
		MinimapCapture->bAlwaysPersistRenderingState = true; // 保留通道信息
		GetWorldTimerManager().SetTimer(MinimapUpdateTimer, this, &ABaseCharacter::UpdateMinimapCapture, 0.05f, true);
	}
}

void ABaseCharacter::UpdateMinimapCapture()
{
	if (MinimapCapture)
	{
		MinimapCapture->CaptureScene();
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

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitAbilityActorInfo();
	OnASCInit();

	if (ASC)
	{
		// 赋予默认值Effect
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DefaultAttrEffect, GetCharacterLevel(), EffectContext);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), ASC);
		}
	}
}

void ABaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
	OnASCInit();
}

void ABaseCharacter::Destroyed()
{
	if (IsLocallyControlled())
	{
		if (UPlayerSubsystem* PlayerSubsystem = ULocalPlayer::GetSubsystem<UPlayerSubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
		{
			PlayerSubsystem->SetIsDead();
		}
		
		GetWorldTimerManager().ClearTimer(MinimapUpdateTimer);
	}

	Super::Destroyed();
}

// 输入设备类型改变
void ABaseCharacter::OnInputMethodChanged(ECommonInputType TempInputType)
{
	// UE_LOG(LogTemp, Warning, TEXT("OnInputMethodChanged: %d"), TempInputType);
	if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
	if (BasePlayerState)
	{
		BasePlayerState->InputType = TempInputType;
	}
	
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
		ASC = Cast<UMAAbilitySystemComponent>(BasePlayerState->GetAbilitySystemComponent());
		if (ASC)
		{
			ASC->InitAbilityActorInfo(BasePlayerState, this);
		}
		AttributeSetBase = BasePlayerState->GetAttributeSetBase();
	}
}

void ABaseCharacter::OnASCInit()
{
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnMaxHealthChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	}
	
	if (IsLocallyControlled())
	{
		if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
		if (BasePlayerState)
		{
			if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
			if (StorageSubsystem && StorageSubsystem->CacheLoadout)
			{
				if (UDNAAsset2* DNAAsset1 = StorageSubsystem->GetDNAAssetByType(StorageSubsystem->CacheLoadout->DNA1))
				{
					if (UDNAAsset2* DNAAsset2 = StorageSubsystem->GetDNAAssetByType(StorageSubsystem->CacheLoadout->DNA2))
					{
						BasePlayerState->ServerSetDNA(DNAAsset1->DNA, DNAAsset2->DNA);
						// BasePlayerState->ServerSetDNA(EDNA::HighBoneDensity, EDNA::SubconsciousAwareness);
					}
				}
			}
		}
	}
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return ASC;
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

float ABaseCharacter::GetBodyResistance()
{
	return AttributeSetBase ? AttributeSetBase->GetBodyResistance() : 0.f;
}

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	FVector2D AxisVector = Value.Get<FVector2D>();

	if (MovementComp && MovementComp->MovementMode == MOVE_Custom && MovementComp->CustomMovementMode == CMOVE_Ladder)
	{
		// 【梯子移动】：将 Y 轴的输入（W和S键）转换为世界坐标的 Z 轴 (UpVector)
		AddMovementInput(FVector::UpVector, AxisVector.Y);
		
		// 利用梯子自身的右方向，而不是玩家的右方向
		if (CurrentLadder)
		{
			FVector PlayerRight = GetActorRightVector();
			FVector LadderRight = CurrentLadder->GetActorRightVector();
			
			// 点乘判断：如果梯子的右方向和玩家视角的右边相反（夹角大于90度），就把向量反转
			// 这保证了无论梯子怎么摆放，你按 D 键永远是往你屏幕的右边走
			if (FVector::DotProduct(PlayerRight, LadderRight) < 0.0f)
			{
				LadderRight = -LadderRight;
			}
			
			AddMovementInput(LadderRight, AxisVector.X);
		}
	}
	else
	{
		AddMovementInput(GetActorForwardVector(), AxisVector.Y);
		AddMovementInput(GetActorRightVector(), AxisVector.X);
	}
}

// 分开处理Look，无需根据输入设备类型区分灵敏度，支持同时使用键鼠和手柄控制一个角色
void ABaseCharacter::LookMouse(const FInputActionValue& Value)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (!StorageSubsystem || !StorageSubsystem->CacheSetting) return;
	FVector2D AxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(AxisVector.X * StorageSubsystem->CacheSetting->MouseSensitivity);
	AddControllerPitchInput(AxisVector.Y * StorageSubsystem->CacheSetting->MouseSensitivity);
}

void ABaseCharacter::LookStick(const FInputActionValue& Value)
{
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (!StorageSubsystem || !StorageSubsystem->CacheSetting) return;
	FVector2D AxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(AxisVector.X * StorageSubsystem->CacheSetting->ControllerSensitivity);
	AddControllerPitchInput(AxisVector.Y * StorageSubsystem->CacheSetting->ControllerSensitivity);
}

void ABaseCharacter::JumpButtonPressed(const FInputActionValue& Value)
{
	if (MovementComp && MovementComp->MovementMode == MOVE_Custom && MovementComp->CustomMovementMode == CMOVE_Ladder)
	{
		// 1. 客户端本地立刻执行脱离（保证手感零延迟）
		ExitLadderAndJump();
		
		// 2. 如果当前是客户端，必须通知服务器也执行一次（防止拉扯/吸回）
		if (!HasAuthority())
		{
			Server_JumpOffLadder();
		}
		return;
	}
	
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

void ABaseCharacter::OnRep_bIsDead()
{
}

void ABaseCharacter::SetHealth(float TempHealth)
{
	if (AttributeSetBase)
	{
		AttributeSetBase->SetHealth(TempHealth);
	}
	
	if (BasePlayerState == nullptr) BasePlayerState = GetPlayerState<ABasePlayerState>();
	if (BasePlayerState)
	{
		BasePlayerState->ForceNetUpdate(); // 立刻响应受伤者血量变化
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
	
	if (IsLocallyControlled())
	{
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
		if (BaseController)
		{
			BaseController->SetHUDHealth(Data.NewValue);
		}
		
		ApplyHurtEffect(Data);
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
	
	if (IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(this, OuchSound, GetActorLocation());
	}
}

// 计算跌落伤害比例
float ABaseCharacter::CalcFallDamageRate()
{
	if (ASC && ASC->HasMatchingGameplayTag(TAG_STATE_DNA_HighBoneDensity))
	{
		return 0.f; 
	}
	
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
	if (!IsLocallyControlled())
	{
		UGameplayStatics::PlaySoundAtLocation(this, OuchSound, GetActorLocation());
	}
}

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

		float VolumeMultiplier;
		USoundAttenuation* DynamicAttenuation;
		GetFootstepAudioSettings(VolumeMultiplier, DynamicAttenuation);

		UGameplayStatics::PlaySoundAtLocation(
			this, 
			Sound, 
			HitResult.Location, 
			VolumeMultiplier, 
			1.0f, 
			0.0f, 
			DynamicAttenuation
		);
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

		float VolumeMultiplier;
		USoundAttenuation* DynamicAttenuation;
		GetFootstepAudioSettings(VolumeMultiplier, DynamicAttenuation);

		UGameplayStatics::PlaySoundAtLocation(
			this, 
			Sound, 
			HitResult.Location, 
			VolumeMultiplier, 
			1.0f, 
			0.0f, 
			DynamicAttenuation
		);
	}
}

void ABaseCharacter::GetFootstepAudioSettings(float& OutVolumeMultiplier, USoundAttenuation*& OutAttenuation)
{
	// 默认值
	OutVolumeMultiplier = 1.0f;
	OutAttenuation = nullptr; 

	if (AssetSubsystem && AssetSubsystem->CharacterAsset)
	{
		OutAttenuation = AssetSubsystem->CharacterAsset->Atten_Footstep_Normal;
	}

	if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ABaseCharacter* LocalCharacter = Cast<ABaseCharacter>(LocalPC->GetPawn()))
		{
			if (LocalCharacter->GetAbilitySystemComponent() && 
				LocalCharacter->GetAbilitySystemComponent()->HasMatchingGameplayTag(TAG_STATE_DNA_EnhancedHearing))
			{
				if (this != LocalCharacter)
				{
					if (LocalCharacter->BasePlayerState == nullptr) LocalCharacter->BasePlayerState = LocalCharacter->GetPlayerState<ABasePlayerState>();
					if (this->BasePlayerState == nullptr) this->BasePlayerState = this->GetPlayerState<ABasePlayerState>();

					if (LocalCharacter->BasePlayerState && this->BasePlayerState)
					{
						if (LocalCharacter->BasePlayerState->Team != this->BasePlayerState->Team)
						{
							OutVolumeMultiplier = 1.5f;
							
							if (AssetSubsystem && AssetSubsystem->CharacterAsset)
							{
								OutAttenuation = AssetSubsystem->CharacterAsset->Atten_Footstep_Enhanced;
							}
						}
					}
				}
			}
		}
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
	if (BaseController && BasePlayerState && AssetSubsystem&& AssetSubsystem->CommonAsset)
	{
		BaseController->ServerSendMsg(
			EMsgType::Radio,
			BasePlayerState->Team,
			BasePlayerState->GetPlayerName(),
			AssetSubsystem->CommonAsset->RadioTexts[RadioIndex]
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
	if (!IsLocallyControlled())
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
	if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(this, AssetSubsystem->CommonAsset->SprayPaintSound))
	{
		// AudioComponent->SetFloatParameter(TEXT("Index"), 1);
	}
	
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * 400.f;

#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 7.f);
#endif

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState)
	{
		QueryParams.AddIgnoredActors(BaseGameState->AllEquipments);
	}

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
		ServerSprayPaint(RadioIndex, OutHit.ImpactPoint, OutHit.ImpactNormal, OutHit.GetComponent(), OutHit.BoneName);
	}
}

void ABaseCharacter::ServerSprayPaint_Implementation(int32 RadioIndex, FVector_NetQuantize ImpactPoint, FVector_NetQuantizeNormal ImpactNormal, UPrimitiveComponent* HitComp, FName BoneName)
{
	MulticastSprayPaint(RadioIndex, ImpactPoint, ImpactNormal, HitComp, BoneName);
}

void ABaseCharacter::MulticastSprayPaint_Implementation(int32 RadioIndex, FVector_NetQuantize ImpactPoint, FVector_NetQuantizeNormal ImpactNormal, UPrimitiveComponent* HitComp, FName BoneName)
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
			
			// 根据法线计算旋转
			FRotator DecalRotation = FRotationMatrix::MakeFromX(ImpactNormal).Rotator();
			DecalRotation.Roll += 90.f;
			DecalRotation.Yaw += 180.f;
			
			// 所有客户端（包括发起者）在相同位置生成贴花
			SprayPaintDecal = UGameplayStatics::SpawnDecalAttached(
				SprayPaints[RadioIndex].Material.LoadSynchronous(),
				FVector(5.f, 100.f, 100.f),
				HitComp,
				BoneName,
				ImpactPoint,
				DecalRotation,
				EAttachLocation::KeepWorldPosition,
				0.f
			);
		}
	}
}

// TODO 改进：血液屏幕方向对应攻击者方向 血液距屏幕边缘距离对应攻击者距离 血液多少对应伤害大小
void ABaseCharacter::ApplyHurtEffect(const FOnAttributeChangeData& Data)
{
	if (!MID_Hurt)
	{
		UAssetSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (Subsystem && Subsystem->CharacterAsset && Subsystem->CharacterAsset->MI_Hurt)
		{
			MID_Hurt = UMaterialInstanceDynamic::Create(Subsystem->CharacterAsset->MI_Hurt, this);
			MID_Hurt->SetVectorParameterValue(FName("BloodColor"), BloodColor);
		}
	}
	if (!MID_Hurt) return;

	// 计算受到的伤害
	float Damage = Data.OldValue - Data.NewValue;
    
	if (Damage > 0.f)
	{
		// 核心修改：如果当前没有血迹（从无到有），则随机生成 UV 偏移量
		if (CurrentBloodIntensity <= 0.f)
		{
			// 生成 0.0 到 1.0 之间的随机偏移值
			float RandomOffsetX = FMath::RandRange(0.0f, 1.0f);
			float RandomOffsetY = FMath::RandRange(0.0f, 1.0f);
			MID_Hurt->SetScalarParameterValue(FName("OffsetX"), RandomOffsetX);
			MID_Hurt->SetScalarParameterValue(FName("OffsetY"), RandomOffsetY);
		}

		// 根据伤害比例增加血迹强度
		float DamageRatio = Damage / GetMaxHealth();
		float AddedBlood = FMath::Clamp(DamageRatio * 4.f, 0.3f, 1.0f);
       
		CurrentBloodIntensity = FMath::Clamp(CurrentBloodIntensity + AddedBlood, 0.0f, 1.0f);

		BloodRecoveryDelayTimer = 3.0f;
	}
}

void ABaseCharacter::UpdateHurtEffect(float DeltaSeconds)
{
	if (!MID_Hurt) return;

	if (BloodRecoveryDelayTimer > 0.f)
	{
		BloodRecoveryDelayTimer -= DeltaSeconds;
	}
	else if (CurrentBloodIntensity > 0.f && !bIsDead)
	{
		// 计时器归零后，才开始随着时间衰减血迹
		CurrentBloodIntensity -= BloodRecoveryRate * DeltaSeconds;
		CurrentBloodIntensity = FMath::Max(CurrentBloodIntensity, 0.0f);
	}

	// 褪色
	float HealthRatio = GetMaxHealth() > 0.f ? (GetHealth() / GetMaxHealth()) : 0.f;
	float DesaturationIntensity = 0.f;

	if (HealthRatio <= 0.2f)
	{
		// 基础衰减比例：血量从 20% 降到 0% 时，BaseRatio 从 0 增加到 1
		float BaseRatio = 1.0f - (HealthRatio / 0.2f);
		BaseRatio = FMath::Clamp(BaseRatio, 0.0f, 1.0f);
		// 乘以 0.4，将最大失色程度限制在 40%
		DesaturationIntensity = BaseRatio * 0.4f;
	}

	MID_Hurt->SetScalarParameterValue(FName("BloodIntensity"), CurrentBloodIntensity);
	MID_Hurt->SetScalarParameterValue(FName("DesaturationIntensity"), DesaturationIntensity);

	if (CurrentBloodIntensity <= 0.f && DesaturationIntensity <= 0.f)
	{
		Camera->AddOrUpdateBlendable(MID_Hurt, 0.f);
	}
	else
	{
		Camera->AddOrUpdateBlendable(MID_Hurt, 1.f);
	}
}

void ABaseCharacter::ApplyFlashbangEffect(float InRadius, float InMaxFlashTime, float InMaxCapTime, float InFlashTime, float InDistance, float InAngle)
{
	if (!Camera) return;
	
	if (!MID_Flashbang)
	{
		UAssetSubsystem* Subsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (Subsystem && Subsystem->CharacterAsset && Subsystem->CharacterAsset->MI_Flashbang)
		{
			MID_Flashbang = UMaterialInstanceDynamic::Create(Subsystem->CharacterAsset->MI_Flashbang, this);
		}
	}
	
	if (!MID_Flashbang) return;
	
	// 更新材质参数
	MID_Flashbang->SetScalarParameterValue(FName("Radius"), InRadius);
	MID_Flashbang->SetScalarParameterValue(FName("MaxFlashTime"), InMaxFlashTime);
	MID_Flashbang->SetScalarParameterValue(FName("MaxCapTime"), InMaxCapTime);
	MID_Flashbang->SetScalarParameterValue(FName("FlashTime"), InFlashTime);
	MID_Flashbang->SetScalarParameterValue(FName("Distance"), InDistance);
	MID_Flashbang->SetScalarParameterValue(FName("Angle"), InAngle);
	
	// 权重设为 1.0f，激活后处理渲染
	Camera->AddOrUpdateBlendable(MID_Flashbang, 1.f);

	// 修复多重闪光弹覆盖 Bug 的核心计算逻辑
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TotalDuration = InMaxFlashTime + InMaxCapTime; // 取相加适当延长时间，避免效果为执行完就关掉了后处理材质
	float ProposedEndTime = CurrentTime + TotalDuration; // 计算这颗新闪光弹理论上的结束时间

	// 只有当新闪光弹的结束时间 > 当前记录的结束时间时，才去刷新定时器
	if (ProposedEndTime > FlashbangEndTime)
	{
		FlashbangEndTime = ProposedEndTime;
		
		// 重新设定定时器，剩余等待时间 = 绝对结束时间 - 当前时间
		GetWorldTimerManager().SetTimer(
			TimerHandle_FlashbangEnd, 
			this, 
			&ABaseCharacter::ClearFlashbangEffect, 
			FlashbangEndTime - CurrentTime, 
			false
		);
	}
}

void ABaseCharacter::ClearFlashbangEffect()
{
	if (Camera && MID_Flashbang)
	{
		Camera->AddOrUpdateBlendable(MID_Flashbang, 0.f);
	}

	FlashbangEndTime = 0.f; 
}

void ABaseCharacter::OnLadderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->ActorHasTag(TAG_LADDER))
	{
		// 如果在冷却期内（刚跳下梯子），拒绝吸附
		if (GetWorld()->GetTimeSeconds() < LadderGrabCooldown)
		{
			return; 
		}

		// 防止顶部退后时被吸附的逻辑
		float TriggerTopZ = OtherComp->Bounds.Origin.Z + OtherComp->Bounds.BoxExtent.Z;
		float PlayerBottomZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (PlayerBottomZ >= TriggerTopZ - 10.0f) return;

		if (HasAuthority() || IsLocallyControlled())
		{
			// 【45度掉落修复 1】：记录当前梯子
			CurrentLadder = OtherActor;
			
			GetCharacterMovement()->SetMovementMode(MOVE_Custom, CMOVE_Ladder);
			GetCharacterMovement()->Velocity = FVector::ZeroVector; 
		}
	}
}

void ABaseCharacter::OnLadderEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->ActorHasTag(TAG_LADDER))
	{
		if (OtherActor == CurrentLadder)
		{
			CurrentLadder = nullptr;
		}

		if (HasAuthority() || IsLocallyControlled())
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			if (GetCharacterMovement()->Velocity.Z > 0)
			{
				FVector PushForward = GetActorForwardVector() * 300.0f;
				GetCharacterMovement()->Velocity += PushForward;
			}
		}
	}
}

void ABaseCharacter::Server_JumpOffLadder_Implementation()
{
	ExitLadderAndJump();
}

void ABaseCharacter::ExitLadderAndJump()
{
	if (MovementComp)
	{
		MovementComp->SetMovementMode(MOVE_Falling);
		// 给一个脱离梯子的初速度
		FVector JumpVelocity = FVector::UpVector * GetJumpZVelocity() + GetActorForwardVector() * -400.f;
		MovementComp->Velocity = JumpVelocity;
	}
	
	// 设置 0.3 秒的梯子吸附冷却期，这样你的身体在被弹开的瞬间，即使还在 TriggerBox 里，也不会再次触发重叠吸附。
	LadderGrabCooldown = GetWorld()->GetTimeSeconds() + 0.3f;
}

#undef LOCTEXT_NAMESPACE
