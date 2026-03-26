#include "BaseGameState.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/GameModes/MutationMode.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/UISubsystem.h"
#include "Net/UnrealNetwork.h"

void ABaseGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Team1PlayerStates);
	DOREPLIFETIME(ThisClass, Team2PlayerStates);
}

void ABaseGameState::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(SetAllEquipmentsTimerHandle, this, &ThisClass::SetAllEquipments, 2.f, true);
}

void ABaseGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	if (MatchState == MatchState::PostRound)
	{
		HandleRoundHasEnded();
	}
}

void ABaseGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		UISubsystem->OnRoundStarted.Broadcast();
	}
}

void ABaseGameState::HandleRoundHasEnded()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		UISubsystem->OnRoundEnded.Broadcast();
	}
}

void ABaseGameState::AddToPlayerStates(ABasePlayerState* BasePlayerState, ETeam Team)
{
	switch (Team)
	{
	case ETeam::Team1:
		Team1PlayerStates.AddUnique(BasePlayerState);
		break;
	case ETeam::Team2:
		Team2PlayerStates.AddUnique(BasePlayerState);
		break;
	}
}

void ABaseGameState::RemoveFromPlayerStates(ABasePlayerState* BasePlayerState, ETeam Team)
{
	switch (Team)
	{
	case ETeam::Team1:
		Team1PlayerStates.Remove(BasePlayerState);
		break;
	case ETeam::Team2:
		Team2PlayerStates.Remove(BasePlayerState);
		break;
	}
}

void ABaseGameState::GetPlayerStates(TOptional<ETeam> Team, TArray<ABasePlayerState*>& OutPlayerStates) const
{
	OutPlayerStates.Reset(); // Reset 会清空数组元素，但不会释放已分配的底层内存

	if (Team.IsSet())
	{
		switch (Team.GetValue())
		{
		case ETeam::Team1:
			OutPlayerStates.Append(Team1PlayerStates);
			break;
		case ETeam::Team2:
			OutPlayerStates.Append(Team2PlayerStates);
			break;
		}
	}
	else
	{
		// 提前分配好总内存，避免 Append 时的二次扩容开销
		OutPlayerStates.Reserve(Team1PlayerStates.Num() + Team2PlayerStates.Num());
		OutPlayerStates.Append(Team1PlayerStates);
		OutPlayerStates.Append(Team2PlayerStates);
	}
}

void ABaseGameState::MulticastAddKillLog_Implementation(ABasePlayerState* AttackerState, const FText& CauserName, ABasePlayerState* DamagedState)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		UISubsystem->OnAddKillLog.Broadcast(AttackerState, CauserName, DamagedState);
	}
}

void ABaseGameState::MulticastSendMsg_Implementation(const EMsgType MsgType, const ETeam Team, const FString& PlayerName, const FString& Msg)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		UISubsystem->OnReceiveMsg.Broadcast(MsgType, Team, PlayerName, Msg);
	}
}

void ABaseGameState::SetAllEquipments()
{
	if (GetWorld())
	{
		AllEquipments.Empty();

		// double Time1 = FPlatformTime::Seconds();

		UGameplayStatics::GetAllActorsWithTag(GetWorld(), TAG_EQUIPMENT, AllEquipments);

		// double Time2 = FPlatformTime::Seconds();
		// UE_LOG(LogTemp, Warning, TEXT("Time %f"), Time2 - Time1);
	}
}
