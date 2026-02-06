#include "MeleeController.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/GameModes/MeleeMode.h"
#include "MutateArena/GameStates/MeleeGameState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/UI/HUD/TeamDeadMatch/TeamDeadMatchHUD.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/UISubsystem.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "AMeleeController"

void AMeleeController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchState);
}

void AMeleeController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		RequestServerMatchInfo();
	}
}

void AMeleeController::RequestServerMatchInfo_Implementation()
{
	if (MeleeMode == nullptr) MeleeMode = Cast<AMeleeMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (MeleeMode)
	{
		ReturnServerMatchInfo(
			MeleeMode->GetMatchState(),
			MeleeMode->LevelStartTime,
			MeleeMode->WarmupTime,
			MeleeMode->MatchTime,
			MeleeMode->CooldownTime
		);
	}
}

void AMeleeController::ReturnServerMatchInfo_Implementation(
	FName TempMatchState,
	float TempLevelStartTime,
	float TempWarmupTime,
	float TempMatchTime,
	float TempCooldownTime
){
	MatchState = TempMatchState;
	HandleMatchStateChange();

	LevelStartTime = TempLevelStartTime;
	WarmupTime = TempWarmupTime;
	MatchTime = TempMatchTime;
	CooldownTime = TempCooldownTime;
}

void AMeleeController::OnMatchStateSet(FName TempMatchState)
{
	MatchState = TempMatchState;
	HandleMatchStateChange();
}

void AMeleeController::OnRep_MatchState()
{
	HandleMatchStateChange();
}

void AMeleeController::HandleMatchStateChange()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::WaitingPostMatch)
	{
		HandleMatchHasEnded();
	}
	else if (MatchState == MatchState::LeavingMap)
	{
		HandleLeavingMap();
	}
}

void AMeleeController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = LevelStartTime + WarmupTime - GetServerTime();
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = LevelStartTime + WarmupTime + MatchTime - GetServerTime();
	}
	else if (MatchState == MatchState::WaitingPostMatch)
	{
		TimeLeft = LevelStartTime + CooldownTime + WarmupTime + MatchTime - GetServerTime();
	}
	int32 SecondsLeft = FMath::RoundToInt(TimeLeft);

	// 当前时间秒数变化时更新HUD
	if (SecondsLeft != CountdownSeconds)
	{
		if (MatchState == MatchState::WaitingToStart)
		{
			SetHUDWarmupCountdown(SecondsLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
			{
				UISubsystem->OnMatchCountdownChange.Broadcast(SecondsLeft);
			}
		}
		else if (MatchState == MatchState::WaitingPostMatch)
		{
		}

		// 记录当前时间秒数
		CountdownSeconds = SecondsLeft;
	}
}

void AMeleeController::InitHUD()
{
	if (MeleeGameState == nullptr) MeleeGameState = Cast<AMeleeGameState>(UGameplayStatics::GetGameState(GetWorld()));
	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetPawn());

	if (MeleeGameState && HumanCharacter)
	{
		SetHUDHealth(HumanCharacter->GetHealth());
		SetHUDTeamScore(MeleeGameState->GetTeamScore(ETeam::Team1), ETeam::Team1);
		SetHUDTeamScore(MeleeGameState->GetTeamScore(ETeam::Team2), ETeam::Team2);
	}
}

void AMeleeController::SetHUDHealth(float Health)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnHumanHealthChange.Broadcast(Health);
	}
}

void AMeleeController::SetHUDTeamScore(float Score, ETeam Team)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnTeamScoreChange.Broadcast(Score, Team);
	}
}

#undef LOCTEXT_NAMESPACE
