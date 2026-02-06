#include "MutationController.h"

#include "AbilitySystemComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameModes/MutationMode.h"
#include "MutateArena/GameStates/MutationGameState.h"
#include "MutateArena/PlayerStates/MutationPlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/UI/HUD/Mutation/MutationMutant.h"
#include "MutateArena/UI/HUD/RadialMenu/RadialMenuContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "MetaSoundSource.h"
#include "Components/AudioComponent.h"
#include "MutateArena/System/UISubsystem.h"

#define LOCTEXT_NAMESPACE "AMutationController"

void AMutationController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchState);
	DOREPLIFETIME(ThisClass, CurRound);
}

void AMutationController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		RequestServerMatchInfo();
	}
}

void AMutationController::RequestServerMatchInfo_Implementation()
{
	if (MutationMode == nullptr) MutationMode = Cast<AMutationMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (MutationMode)
	{
		ReturnServerMatchInfo(
			MutationMode->GetMatchState(),
			MutationMode->TotalRound,
			MutationMode->LevelStartTime,
			MutationMode->WarmupTime,
			MutationMode->RoundStartTime,
			MutationMode->RoundTime,
			MutationMode->MutateTime,
			MutationMode->RoundEndTime,
			MutationMode->PostRoundTime,
			MutationMode->CooldownTime
		);
	}
}

void AMutationController::ReturnServerMatchInfo_Implementation(
	FName TempMatchState,
	int32 TempTotalRound,
	float TempLevelStartTime,
	float TempWarmupTime,
	float TempRoundStartTime,
	float TempRoundTime,
	float TempMutateTime,
	float TempRoundEndTime,
	float TempPostRoundTime,
	float TempCooldownTime
){
	MatchState = TempMatchState;
	HandleMatchStateChange();

	TotalRound = TempTotalRound;
	SetHUDTotalRound();

	LevelStartTime = TempLevelStartTime;
	WarmupTime = TempWarmupTime;
	RoundStartTime = TempRoundStartTime;
	RoundTime = TempRoundTime;
	MutateTime = TempMutateTime;
	RoundEndTime = TempRoundEndTime;
	PostRoundTime = TempPostRoundTime;
	CooldownTime = TempCooldownTime;
}

void AMutationController::OnMatchStateSet(FName TempMatchState, int32 TempCurRound)
{
	MatchState = TempMatchState;
	HandleMatchStateChange();

	CurRound = TempCurRound;
	if (IsLocalController())
	{
		SetHUDCurRound();
	}
}

void AMutationController::OnRep_MatchState()
{
	HandleMatchStateChange();
}

void AMutationController::HandleMatchStateChange()
{
	if (MatchState == MatchState::InProgress)
	{
		RoundStartTime = GetServerTime();
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::PostRound)
	{
		RoundEndTime = GetServerTime();
		HandleRoundHasEnded();
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

void AMutationController::HandleRoundHasEnded()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->ChangeAnnouncement.Broadcast(LOCTEXT("RoundEnd", "Round end"));
	}
}

void AMutationController::OnRep_CurRound()
{
	SetHUDCurRound();
}

void AMutationController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = LevelStartTime + WarmupTime - GetServerTime();
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = RoundStartTime + RoundTime - GetServerTime();
	}
	else if (MatchState == MatchState::PostRound)
	{
		TimeLeft = RoundEndTime + PostRoundTime - GetServerTime();
	}
	else if (MatchState == MatchState::WaitingPostMatch)
	{
		TimeLeft = RoundEndTime + PostRoundTime + CooldownTime - GetServerTime();
	}
	int32 SecondsLeft = FMath::RoundToInt(TimeLeft);

	// 当前时间秒数变化时更新HUD
	if (SecondsLeft != CountdownSeconds)
	{
		if (MatchState == MatchState::WaitingToStart)
		{
			SetHUDWarmupCountdown(SecondsLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
			{
				UISubsystem->OnMatchCountdownChange.Broadcast(SecondsLeft);
			}
			
			// 突变倒计时
			int32 MutateCountdown = FMath::RoundToInt(MutateTime + TimeLeft - RoundTime);
			if (MutateCountdown > 0)
			{
				// 玩家中途加入需待ABaseController::ReturnServerTime和AMutationController::ReturnServerMatchInfo已完成
				// 不然MutateCountdown计算是错误的
				if (!bServerClientDeltaTimeHasInit || TotalRound == 0) return;
				
				if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
				{
					UISubsystem->ChangeAnnouncement.Broadcast(FText::FromString(FString::Printf(TEXT("%d"), MutateCountdown)));
				}

				// 播放倒计时
				if (MutateCountdown <= 10 && MutateCountdown > 0)
				{
					if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
					if (AssetSubsystem && AssetSubsystem->CommonAsset)
					{
						if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(this, AssetSubsystem->CommonAsset->CountdownSound))
						{
							AudioComponent->SetFloatParameter(TEXT("Index"), MutateCountdown - 1);
						}
					}
				}
			}
			else if (MutateCountdown == 0)
			{
				if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
				{
					UISubsystem->ChangeAnnouncement.Broadcast(FText());
				}
			}
		}
		else if (MatchState == MatchState::WaitingPostMatch)
		{
		}

		// 记录当前时间秒数
		CountdownSeconds = SecondsLeft;
	}
}

void AMutationController::InitHUD()
{
	Super::InitHUD();

	if (MutationPlayerState == nullptr) MutationPlayerState = Cast<AMutationPlayerState>(PlayerState);
	if (MutationPlayerState)
	{
		if (MutationPlayerState->Team == ETeam::Team1)
		{
			InitHumanHUD();
		}
		else if(MutationPlayerState->Team == ETeam::Team2)
		{
			InitMutantHUD();
		}

		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
		{
			UISubsystem->OnTeamChange.Broadcast(MutationPlayerState->Team);
		}
	}
}

void AMutationController::InitHumanHUD()
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if (MutationGameState == nullptr) MutationGameState = Cast<AMutationGameState>(UGameplayStatics::GetGameState(GetWorld()));

	if (BaseCharacter && MutationGameState)
	{
		SetHUDHealth(BaseCharacter->GetHealth());
		SetHUDCurRound();
		SetHUDTeamNum(MutationGameState->GetPlayerStates(ETeam::Team1).Num(), ETeam::Team1);
		SetHUDTeamNum(MutationGameState->GetPlayerStates(ETeam::Team2).Num(), ETeam::Team2);
		SetHUDDamageMul(MutationGameState->DamageMul);
	}
}

void AMutationController::InitMutantHUD()
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if (MutationGameState == nullptr) MutationGameState = Cast<AMutationGameState>(UGameplayStatics::GetGameState(GetWorld()));
	if (MutationPlayerState == nullptr) MutationPlayerState = Cast<AMutationPlayerState>(PlayerState);

	if (BaseCharacter && MutationGameState && MutationPlayerState)
	{
		SetHUDHealth(BaseCharacter->GetHealth());
		SetHUDCurRound();
		SetHUDTeamNum(MutationGameState->GetPlayerStates(ETeam::Team1).Num(), ETeam::Team1);
		SetHUDTeamNum(MutationGameState->GetPlayerStates(ETeam::Team2).Num(), ETeam::Team2);
		SetHUDRage(MutationPlayerState->Rage);

		if (MutationPlayerState->GetAbilitySystemComponent())
		{
			FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TAG_MUTANT_SKILL_CD);
			int32 TagCount = MutationPlayerState->GetAbilitySystemComponent()->GetTagCount(Tag);
			ShowHUDSkill(TagCount == 0.f && MutationPlayerState->GetCharacterLevel() >= 2.f);
		}
	}
}

void AMutationController::SetHUDHealth(float Health)
{
	if (MutationPlayerState == nullptr) MutationPlayerState = Cast<AMutationPlayerState>(PlayerState);
	if (MutationPlayerState)
	{
		if (MutationPlayerState->Team == ETeam::Team1)
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
			{
				UISubsystem->OnHumanHealthChange.Broadcast(Health);
			}
		}
		else if(MutationPlayerState->Team == ETeam::Team2)
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
			{
				UISubsystem->OnMutantHealthChange.Broadcast(Health);
			}
		}
	}
}

void AMutationController::SetHUDTeamNum(int32 TeamNum, ETeam Team)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnTeamNumChange.Broadcast(TeamNum, Team);
	}
}

void AMutationController::SetHUDCurRound()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnCurRoundChange.Broadcast(CurRound);
	}
}

void AMutationController::SetHUDTotalRound()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnTotalRoundChange.Broadcast(TotalRound);
	}
}

void AMutationController::ShowHUDSkill(bool bIsShow)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnSkillChange.Broadcast(bIsShow);
	}
}

void AMutationController::SetHUDRage(float Rage)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnRageChange.Broadcast(Rage);
	}
}

void AMutationController::SetHUDDamageMul(float DamageMul)
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetLocalPlayer()))
	{
		UISubsystem->OnDamageMulChange.Broadcast(DamageMul);
	}
}

void AMutationController::MulticastPlaySpawnPickupSound_Implementation()
{
	if (!IsLocalController()) return;
	
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(this, AssetSubsystem->CommonAsset->SpawnPickupSound))
		{
		}
	}
}

#undef LOCTEXT_NAMESPACE
