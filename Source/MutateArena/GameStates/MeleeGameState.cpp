#include "MeleeGameState.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/GameModes/MeleeMode.h"
#include "MutateArena/PlayerControllers/MeleeController.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/PlayerStates/TeamType.h"

AMeleeGameState::AMeleeGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Tags.Add(TAG_GAME_STATE_MELEE);
}

void AMeleeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Team1PlayerStates);
	DOREPLIFETIME(ThisClass, Team2PlayerStates);
	DOREPLIFETIME(ThisClass, Team1Score);
	DOREPLIFETIME(ThisClass, Team2Score);
}

void AMeleeGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		WatchGameState();
	}
}

void AMeleeGameState::WatchGameState()
{
	// 监视比赛状态
	if (MeleeMode == nullptr) MeleeMode = Cast<AMeleeMode>(GetWorld()->GetAuthGameMode());
	if (MeleeMode && MeleeMode->bWatchMatchState)
	{
		if (Team1PlayerStates.IsEmpty() || Team2PlayerStates.IsEmpty())
		{
			MeleeMode->EndMatch();
		}
	}
}

float AMeleeGameState::GetTeamScore(ETeam Team)
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

void AMeleeGameState::AddTeamScore(ETeam Team)
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

void AMeleeGameState::OnRep_Team1Score()
{
	SetHUDTeamScore(Team1Score, ETeam::Team1);
}

void AMeleeGameState::OnRep_Team2Score()
{
	SetHUDTeamScore(Team2Score, ETeam::Team2);
}

void AMeleeGameState::SetHUDTeamScore(float Score, ETeam Team)
{
	if (MeleeController == nullptr) MeleeController = Cast<AMeleeController>(GetWorld()->GetFirstPlayerController());
	if (MeleeController)
	{
		MeleeController->SetHUDTeamScore(Score, Team);
	}
}
