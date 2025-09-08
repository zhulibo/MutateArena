#include "SpectatorCharacter.h"

#include "CommonInputSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "Data/InputAsset.h"
#include "GameFramework/GameMode.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"

void ASpectatorCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASpectatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->InputAsset == nullptr || AssetSubsystem->InputAsset == nullptr) return;
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(AssetSubsystem->InputAsset->BaseMappingContext, 10);
			Subsystem->AddMappingContext(AssetSubsystem->InputAsset->SpectatorMappingContext, 20);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::LookMouse);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->LookStickAction, ETriggerEvent::Triggered, this, &ThisClass::LookStick);
		
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->ScoreboardAction, ETriggerEvent::Triggered, this, &ThisClass::ScoreboardButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->ScoreboardAction, ETriggerEvent::Completed, this, &ThisClass::ScoreboardButtonReleased);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->PauseMenuAction, ETriggerEvent::Triggered, this, &ThisClass::PauseMenuButtonPressed);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->TextChatAction, ETriggerEvent::Triggered, this, &ThisClass::TextChat);
		
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->SwitchPerspectiveAction, ETriggerEvent::Triggered, this, &ThisClass::SwitchPerspective);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->ViewNextAction, ETriggerEvent::Triggered, this, &ThisClass::ViewNextPlayer);
		EnhancedInputComponent->BindAction(AssetSubsystem->InputAsset->ViewPrevAction, ETriggerEvent::Triggered, this, &ThisClass::ViewPrevPlayer);
	}
}

void ASpectatorCharacter::SwitchPerspective(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		if (PerspectiveType == EPerspectiveType::FirstPerson)
		{
			PerspectiveType = EPerspectiveType::Free;
			BaseController->ServerViewSelf();
		}
		else
		{
			PerspectiveType = EPerspectiveType::FirstPerson;
			BaseController->ServerViewPrevPlayer();
		}
	}
}

void ASpectatorCharacter::ViewPrevPlayer(const FInputActionValue& Value)
{
	// 游戏未开始无法切换玩家
	if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState && BaseGameState->GetMatchState() == MatchState::WaitingToStart)
	{
		return;
	}

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		PerspectiveType = EPerspectiveType::FirstPerson;
		BaseController->ServerViewPrevPlayer();
	}
}

void ASpectatorCharacter::ViewNextPlayer(const FInputActionValue& Value)
{
	// 游戏未开始无法切换玩家
	if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState && BaseGameState->GetMatchState() == MatchState::WaitingToStart)
	{
		return;
	}

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		PerspectiveType = EPerspectiveType::FirstPerson;
		BaseController->ServerViewNextPlayer();
	}
}

void ASpectatorCharacter::LookMouse(const FInputActionValue& Value)
{
	FVector2D AxisVector = Value.Get<FVector2D>();
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	AddControllerYawInput(AxisVector.X * StorageSubsystem->CacheSetting->MouseSensitivity);
	AddControllerPitchInput(AxisVector.Y * StorageSubsystem->CacheSetting->MouseSensitivity);
}

void ASpectatorCharacter::LookStick(const FInputActionValue& Value)
{
	FVector2D AxisVector = Value.Get<FVector2D>();
	if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	AddControllerYawInput(AxisVector.X * StorageSubsystem->CacheSetting->ControllerSensitivity);
	AddControllerPitchInput(AxisVector.Y * StorageSubsystem->CacheSetting->ControllerSensitivity);
}

void ASpectatorCharacter::ScoreboardButtonPressed(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowScoreboard.Broadcast(true);
	}
}

void ASpectatorCharacter::ScoreboardButtonReleased(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowScoreboard.Broadcast(false);
	}
}

void ASpectatorCharacter::PauseMenuButtonPressed(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		BaseController->ShowPauseMenu.Broadcast();
	}
}

void ASpectatorCharacter::TextChat(const FInputActionValue& Value)
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
