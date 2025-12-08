#include "BaseController.h"

#include "EnhancedInputSubsystems.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/PlayerSubsystem.h"
#include "MutateArena/UI/GameLayout.h"
#include "MutateArena/UI/HUD/Mutation/MutationContainer.h"
#include "MutateArena/UI/HUD/Melee/MeleeContainer.h"
#include "MutateArena/UI/HUD/TeamDeadMatch/TeamDeadMatchContainer.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "GameFramework/PlayerState.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#define LOCTEXT_NAMESPACE "ABaseController"

void ABaseController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		HandleServerClientDeltaTime();
	
		if (UPlayerSubsystem* PlayerSubsystem = ULocalPlayer::GetSubsystem<UPlayerSubsystem>(GetLocalPlayer()))
		{
			PlayerSubsystem->AddNotifyLayout();
		}

		AddGameLayout();
	}
}

void ABaseController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocalController())
	{
		SetHUDTime();
	}
}

void ABaseController::OnUnPossess()
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->InputAsset == nullptr || AssetSubsystem->InputAsset == nullptr) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->BaseMappingContext);
		Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->SpectatorMappingContext);
		Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->HumanMappingContext);
		Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->MutantMappingContext);
		Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->RadialMenuMappingContext);
	}

	Super::OnUnPossess();
}

void ABaseController::HandleServerClientDeltaTime()
{
	// 周期性获取ServerClientDeltaTime
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]() {
		RequestServerTime(GetWorld()->GetTimeSeconds());
	});
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 5.f, true, 0.f);
}

void ABaseController::RequestServerTime_Implementation(float ClientTime)
{
	ReturnServerTime(ClientTime, GetWorld()->GetTimeSeconds());
}

void ABaseController::ReturnServerTime_Implementation(float ClientTime, float ServerTime)
{
	float RoundTripNetworkDelay = GetWorld()->GetTimeSeconds() - ClientTime; // 往返网络延迟
	ServerClientDeltaTime = ServerTime - ClientTime - RoundTripNetworkDelay * 0.5f;

	bServerClientDeltaTimeHasInit = true;
}

float ABaseController::GetServerTime()
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	return GetWorld()->GetTimeSeconds() + ServerClientDeltaTime;
}

void ABaseController::AddGameLayout()
{
	GameLayout = CreateWidget<UGameLayout>(this, GameLayoutClass);
	if (GameLayout)
	{
		GameLayout->AddToViewport();

		FocusGame();

		if (GetWorld()->GetGameState()->ActorHasTag(TAG_GAME_STATE_MUTATION))
		{
			GameLayout->GameStack->AddWidget(GameLayout->MutationContainerClass);
		}
		else if (GetWorld()->GetGameState()->ActorHasTag(TAG_GAME_STATE_MELEE))
		{
			GameLayout->GameStack->AddWidget(GameLayout->MeleeContainerClass);
		}
		else if (GetWorld()->GetGameState()->ActorHasTag(TAG_GAME_STATE_TDM))
		{
			GameLayout->GameStack->AddWidget(GameLayout->TeamDeadMatchContainerClass);
		}
	}
}

void ABaseController::FocusGame()
{
	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);
	SetShowMouseCursor(false);
}

void ABaseController::FocusUI()
{
	FInputModeUIOnly InputModeData;
	SetInputMode(InputModeData);
	SetShowMouseCursor(true);
}

void ABaseController::SetPlayerSpectate()
{
	if (HasAuthority())
	{
		PlayerState->SetIsSpectator(true);
		ChangeState(NAME_Spectating);
		bPlayerIsWaiting = true;

		ClientGotoState(NAME_Spectating);
		ClientHUDStateChanged(EHUDState::Spectating);
	}
}

void ABaseController::SetPlayerPlay()
{
	if (HasAuthority())
	{
		PlayerState->SetIsSpectator(false);
		ChangeState(NAME_Playing);
		bPlayerIsWaiting = false;

		ClientGotoState(NAME_Playing);
		ClientHUDStateChanged(EHUDState::Playing);
	}
}

void ABaseController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	// if (IsInState(NAME_Spectating))
	// {
	// 	// TODO 服务端实现自动切到玩家视角不成功
	// 	ServerViewNextPlayer();
	// }
}

void ABaseController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	
	// if (IsInState(NAME_Spectating))
	// {
	// 	ServerViewNextPlayer();
	// }
}

void ABaseController::SetViewTarget(class AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams)
{
	Super::SetViewTarget(NewViewTarget, TransitionParams);

	OnViewTargetChange.Broadcast(NewViewTarget);
}

void ABaseController::ClientHUDStateChanged_Implementation(EHUDState HUDState)
{
	OnHUDStateChange.Broadcast(HUDState);
}

void ABaseController::SetHUDWarmupCountdown(int32 CountdownTime)
{
	FString String = ULibraryCommon::GetFormatTime(CountdownTime);
	ChangeAnnouncement.Broadcast(FText::Format(LOCTEXT("GameStart", "Game starts in {0}"), FText::FromString(String)));
}

void ABaseController::HandleMatchHasStarted()
{
	ChangeAnnouncement.Broadcast(FText());
}

void ABaseController::HandleMatchHasEnded()
{
	OnMatchEnd.Broadcast();
	
	ChangeAnnouncement.Broadcast(LOCTEXT("GameOver", "Game over"));
}

void ABaseController::HandleLeavingMap()
{
	if (IsLocalController() && !HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleLeavingMap ------------------------------------------"));
		ClientTravel(MAP_MENU, ETravelType::TRAVEL_Absolute);
	}
}

void ABaseController::SetHUDAmmo(int32 Ammo)
{
	OnAmmoChange.Broadcast(Ammo);
}

void ABaseController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	OnCarriedAmmoChange.Broadcast(CarriedAmmo);
}

void ABaseController::ServerSendMsg_Implementation(EMsgType MsgType, ETeam Team, const FString& PlayerName, const FString& Msg)
{
	if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState)
	{
		BaseGameState->MulticastSendMsg(MsgType, Team, PlayerName, Msg);
	}
}

#undef LOCTEXT_NAMESPACE
