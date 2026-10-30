#include "SpectatorCharacter.h"

#include "CommonInputSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "Data/InputAsset.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/System/UISubsystem.h"
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
	if (AssetSubsystem == nullptr) return;
	
	UInputAsset* InputAsset = AssetSubsystem->InputAsset;
	if (!InputAsset) return;
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputAsset->BaseMappingContext, 10);
			Subsystem->AddMappingContext(InputAsset->SpectatorMappingContext, 20);
		}
	}

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(InputAsset->LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::LookMouse);
		EIC->BindAction(InputAsset->LookStickAction, ETriggerEvent::Triggered, this, &ThisClass::LookStick);
		
		EIC->BindAction(InputAsset->ScoreboardAction, ETriggerEvent::Triggered, this, &ThisClass::ScoreboardButtonPressed);
		EIC->BindAction(InputAsset->ScoreboardAction, ETriggerEvent::Completed, this, &ThisClass::ScoreboardButtonReleased);
		EIC->BindAction(InputAsset->PauseMenuAction, ETriggerEvent::Triggered, this, &ThisClass::PauseMenuButtonPressed);
		EIC->BindAction(InputAsset->TextChatAction, ETriggerEvent::Triggered, this, &ThisClass::TextChat);
		
		EIC->BindAction(InputAsset->SwitchPerspectiveAction, ETriggerEvent::Triggered, this, &ThisClass::SwitchPerspective);
		EIC->BindAction(InputAsset->ViewNextAction, ETriggerEvent::Triggered, this, &ThisClass::ViewNextPlayer);
		EIC->BindAction(InputAsset->ViewPrevAction, ETriggerEvent::Triggered, this, &ThisClass::ViewPrevPlayer);
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
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->ShowScoreboard.Broadcast(true);
		}
	}
}

void ASpectatorCharacter::ScoreboardButtonReleased(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->ShowScoreboard.Broadcast(false);
		}
	}
}

void ASpectatorCharacter::PauseMenuButtonPressed(const FInputActionValue& Value)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(Controller);
	if (BaseController)
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->ShowPauseMenu.Broadcast();
		}
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
		
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->ShowTextChat.Broadcast();
		}
	}
}
