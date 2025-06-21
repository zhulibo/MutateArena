#include "TeamDeadMatchGameState.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/GameModes/TeamDeadMatchMode.h"
#include "MutateArena/PlayerControllers/TeamDeadMatchController.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/PlayerStates/TeamType.h"

ATeamDeadMatchGameState::ATeamDeadMatchGameState()
{
	PrimaryActorTick.bCanEverTick = true;

	Tags.Add(TAG_GAME_STATE_TDM);
}

void ATeamDeadMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Team1PlayerStates);
	DOREPLIFETIME(ThisClass, Team2PlayerStates);
	DOREPLIFETIME(ThisClass, Team1Score);
	DOREPLIFETIME(ThisClass, Team2Score);
}

void ATeamDeadMatchGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		WatchGameState();
	}
}

void ATeamDeadMatchGameState::WatchGameState()
{
	// 监视比赛状态
	if (TeamDeadMatchMode == nullptr) TeamDeadMatchMode = Cast<ATeamDeadMatchMode>(GetWorld()->GetAuthGameMode());
	if (TeamDeadMatchMode && TeamDeadMatchMode->bWatchMatchState)
	{
		if (Team1PlayerStates.IsEmpty() || Team2PlayerStates.IsEmpty())
		{
			TeamDeadMatchMode->EndMatch();
		}
	}
}

float ATeamDeadMatchGameState::GetTeamScore(ETeam Team)
{
	switch (Team)
	{
	case ETeam::Team1:
		return Team1Score;
	case ETeam::Team2:
		return Team2Score;
	default:
		return 0;
	}
}

void ATeamDeadMatchGameState::AddTeamScore(ETeam Team)
{
	switch (Team)
	{
	case ETeam::Team1:
		Team1Score++;
		SetHUDTeamScore(Team1Score, Team);
		break;
	case ETeam::Team2:
		Team2Score++;
		SetHUDTeamScore(Team2Score, Team);
		break;
	}
}

void ATeamDeadMatchGameState::OnRep_Team1Score()
{
	SetHUDTeamScore(Team1Score, ETeam::Team1);
}

void ATeamDeadMatchGameState::OnRep_Team2Score()
{
	SetHUDTeamScore(Team2Score, ETeam::Team2);
}

void ATeamDeadMatchGameState::SetHUDTeamScore(float Score, ETeam Team)
{
	if (TeamDeadMatchController == nullptr) TeamDeadMatchController = Cast<ATeamDeadMatchController>(GetWorld()->GetFirstPlayerController());
	if (TeamDeadMatchController)
	{
		TeamDeadMatchController->SetHUDTeamScore(Score, Team);
	}
}
