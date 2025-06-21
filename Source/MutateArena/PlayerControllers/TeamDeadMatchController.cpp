#include "TeamDeadMatchController.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/GameModes/TeamDeadMatchMode.h"
#include "MutateArena/GameStates/TeamDeadMatchGameState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/UI/HUD/TeamDeadMatch/TeamDeadMatchHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ATeamDeadMatchController"

void ATeamDeadMatchController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchState);
}

void ATeamDeadMatchController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		RequestServerMatchInfo();
	}
}

void ATeamDeadMatchController::RequestServerMatchInfo_Implementation()
{
	if (TeamDeadMatchMode == nullptr) TeamDeadMatchMode = Cast<ATeamDeadMatchMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (TeamDeadMatchMode)
	{
		ReturnServerMatchInfo(
			TeamDeadMatchMode->GetMatchState(),
			TeamDeadMatchMode->LevelStartTime,
			TeamDeadMatchMode->WarmupTime,
			TeamDeadMatchMode->MatchTime,
			TeamDeadMatchMode->CooldownTime
		);
	}
}

void ATeamDeadMatchController::ReturnServerMatchInfo_Implementation(
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

void ATeamDeadMatchController::OnMatchStateSet(FName TempMatchState)
{
	MatchState = TempMatchState;
	HandleMatchStateChange();
}

void ATeamDeadMatchController::OnRep_MatchState()
{
	HandleMatchStateChange();
}

void ATeamDeadMatchController::HandleMatchStateChange()
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

void ATeamDeadMatchController::SetHUDTime()
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
			OnMatchCountdownChange.Broadcast(SecondsLeft);
		}
		else if (MatchState == MatchState::WaitingPostMatch)
		{
		}

		// 记录当前时间秒数
		CountdownSeconds = SecondsLeft;
	}
}

void ATeamDeadMatchController::InitHUD()
{
	if (TeamDeadMatchGameState == nullptr) TeamDeadMatchGameState = Cast<ATeamDeadMatchGameState>(UGameplayStatics::GetGameState(GetWorld()));
	if (HumanCharacter == nullptr) HumanCharacter = Cast<AHumanCharacter>(GetPawn());

	if (TeamDeadMatchGameState && HumanCharacter)
	{
		SetHUDHealth(HumanCharacter->GetMaxHealth());
		SetHUDTeamScore(TeamDeadMatchGameState->GetTeamScore(ETeam::Team1), ETeam::Team1);
		SetHUDTeamScore(TeamDeadMatchGameState->GetTeamScore(ETeam::Team2), ETeam::Team2);
	}
}

void ATeamDeadMatchController::SetHUDHealth(float Health)
{
	OnHumanHealthChange.Broadcast(Health);
}

void ATeamDeadMatchController::SetHUDTeamScore(float Score, ETeam Team)
{
	OnTeamScoreChange.Broadcast(Score, Team);
}

#undef LOCTEXT_NAMESPACE
