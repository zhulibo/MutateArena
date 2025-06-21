#include "TeamDeadMatchMode.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerControllers/TeamDeadMatchController.h"
#include "MutateArena/PlayerStates/TeamDeadMatchPlayerState.h"
#include "MutateArena/GameStates/TeamDeadMatchGameState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/DevSetting.h"

void ATeamDeadMatchMode::BeginPlay()
{
	Super::BeginPlay();

	// 不包括Standalone
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			WarmupTime = GetDefault<UDevSetting>()->WarmupTime;
		}
	}
}

void ATeamDeadMatchMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 比赛前
	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = LevelStartTime + WarmupTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	// 比赛中
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = LevelStartTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f)
		{
			EndMatch();
		}
	}
	// 比赛后
	else if (MatchState == MatchState::WaitingPostMatch)
	{
		CountdownTime = MatchEndTime + CooldownTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f)
		{
			StartToLeaveMap();
		}
	}
}

void ATeamDeadMatchMode::EndMatch()
{
	if (bIsEndingMatch) return;
	bIsEndingMatch = true;

	// 比赛时间结束时结束监视比赛状态
	bWatchMatchState = false;
	
	if (TeamDeadMatchGameState == nullptr) TeamDeadMatchGameState = GetGameState<ATeamDeadMatchGameState>();
	if (TeamDeadMatchGameState)
	{
		TeamDeadMatchGameState->bCanSpectate = false;
	}

	MatchEndTime = GetWorld()->GetTimeSeconds();

	Super::EndMatch();
}

// MatchState变化时通知所有Controller
void ATeamDeadMatchMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ATeamDeadMatchController* TeamDeadMatchController = Cast<ATeamDeadMatchController>(*It))
		{
			TeamDeadMatchController->OnMatchStateSet(MatchState);
		}
	}
}

// 游戏开始
void ATeamDeadMatchMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// 生成角色
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ATeamDeadMatchController* TeamDeadMatchController = Cast<ATeamDeadMatchController>(*It))
		{
			HandleSpawn(TeamDeadMatchController);
		}
	}

	// 角色生成后开始监视比赛状态
	bWatchMatchState = true;

	GetWorld()->GetTimerManager().SetTimer(ChangeLobbyStatusTimerHandle, this, &ThisClass::HandleChangeLobbyStatus,60.f, true);

	if (TeamDeadMatchGameState == nullptr) TeamDeadMatchGameState = GetGameState<ATeamDeadMatchGameState>();
	if (TeamDeadMatchGameState)
	{
		TeamDeadMatchGameState->bCanSpectate = true;
	}
}

void ATeamDeadMatchMode::HandleSpawn(AController* Controller)
{
	if (TeamDeadMatchGameState == nullptr) TeamDeadMatchGameState = GetGameState<ATeamDeadMatchGameState>();
	if (TeamDeadMatchGameState)
	{
		// TODO 从EOS中获得队伍 ABasePlayerState::ServerSetAccountId
		ETeam Team = TeamDeadMatchGameState->GetPlayerStates(ETeam::Team1).Num() > TeamDeadMatchGameState->GetPlayerStates(ETeam::Team2).Num() ? ETeam::Team2 : ETeam::Team1;
		AssignTeam(Controller, Team);
	}

	SpawnHumanCharacter(Controller);
}

// 中途加入
void ATeamDeadMatchMode::OnPostLogin(AController* Controller)
{
	Super::OnPostLogin(Controller);

	// 生成角色
	if (MatchState == MatchState::InProgress)
	{
		HandleSpawn(Controller);
	}
}

// 角色受到伤害
void ATeamDeadMatchMode::HumanReceiveDamage(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
	float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser)
{
	if (DamagedCharacter == nullptr || DamagedController == nullptr || AttackerController == nullptr || DamageCauser == nullptr) return;

	ATeamDeadMatchPlayerState* DamagedState = Cast<ATeamDeadMatchPlayerState>(DamagedController->PlayerState);
	ATeamDeadMatchPlayerState* AttackerState = Cast<ATeamDeadMatchPlayerState>(AttackerController->PlayerState);

	if (DamagedState == nullptr || AttackerState == nullptr) return;

	// 设置受伤者血量
	float TakenDamage = FMath::Clamp(Damage, 0.f, DamagedCharacter->GetHealth());
	float Health = DamagedCharacter->GetHealth() - TakenDamage;
	DamagedCharacter->SetHealth(Health);
	DamagedCharacter->MulticastSetHealth(Health, AttackerController);

	// 增加攻击者伤害分数
	if (AttackerState != DamagedState) // 受到跌落伤害时，AttackerController和DamageCauser传的是自己
	{
		AttackerState->AddDamage(TakenDamage);
	}

	// 角色死亡
	if (DamagedCharacter->GetHealth() <= 0.f)
	{
		if (AttackerState != DamagedState)
		{
			// 增加攻击者所在队伍的分数
			if (TeamDeadMatchGameState == nullptr) TeamDeadMatchGameState = GetGameState<ATeamDeadMatchGameState>();
			if (TeamDeadMatchGameState)
			{
				TeamDeadMatchGameState->AddTeamScore(AttackerState->Team);
			}

			// 增加攻击者连杀
			AttackerState->AddKillStreak();
		}

		// 击杀日志
		AddKillLog(AttackerState, DamageCauser, DamageType, DamagedState);

		// 增加受伤者死亡次数
		DamagedState->AddDefeat();

		// 处理受伤者死亡流程
		DamagedCharacter->MulticastTeamDeadMatchDead();
	}
}

// 角色重生
void ATeamDeadMatchMode::HumanRespawn(ACharacter* Character, AController* Controller)
{
	if (Character && Controller)
	{
		Character->Destroy();

		SpawnHumanCharacter(Controller);
	}
}

void ATeamDeadMatchMode::HandleChangeLobbyStatus()
{
	float Time = GetWorld()->GetTimeSeconds() - LevelStartTime - WarmupTime;
	int64 Minute = Time / 60 + 1;
	ChangeLobbyStatus(Minute);
}
