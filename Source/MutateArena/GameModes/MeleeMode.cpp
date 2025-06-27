#include "MeleeMode.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/PlayerControllers/MeleeController.h"
#include "MutateArena/PlayerStates/MeleePlayerState.h"
#include "MutateArena/GameStates/MeleeGameState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/System/EOSSubsystem.h"

void AMeleeMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			WarmupTime = GetDefault<UDevSetting>()->WarmupTime;
		}
	}
}

void AMeleeMode::Tick(float DeltaSeconds)
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

void AMeleeMode::EndMatch()
{
	if (bIsEndingMatch) return;
	bIsEndingMatch = true;

	// 比赛时间结束时结束监视比赛状态
	bWatchMatchState = false;

	MatchEndTime = GetWorld()->GetTimeSeconds();

	Super::EndMatch();
}

// MatchState变化时通知所有Controller
void AMeleeMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMeleeController* MeleeController = Cast<AMeleeController>(*It))
		{
			MeleeController->OnMatchStateSet(MatchState);
		}
	}
}

// 游戏开始
void AMeleeMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// 生成角色
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMeleeController* MeleeController = Cast<AMeleeController>(*It))
		{
			HandleSpawn(MeleeController);
		}
	}

	// 角色生成后开始监视比赛状态
	bWatchMatchState = true;

	GetWorld()->GetTimerManager().SetTimer(ChangeLobbyStatusTimerHandle, this, &ThisClass::HandleChangeLobbyStatus,60.f, true);
}

void AMeleeMode::HandleSpawn(AController* Controller)
{
	if (MeleeGameState == nullptr) MeleeGameState = GetGameState<AMeleeGameState>();
	if (MeleeGameState)
	{
		ETeam Team = ETeam::NoTeam;

		if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
		if (EOSSubsystem)
		{
			Team = EOSSubsystem->GetMemberTeam(EOSSubsystem->GetMemberByPlayerName(Controller->PlayerState->GetPlayerName()));
		}

		if (Team == ETeam::NoTeam)
		{
			Team = MeleeGameState->GetPlayerStates(ETeam::Team1).Num() > MeleeGameState->GetPlayerStates(ETeam::Team2).Num() ? ETeam::Team2 : ETeam::Team1;
		}

		AssignTeam(Controller, Team);
	}

	SpawnHumanCharacter(Controller);
}

// 中途加入
void AMeleeMode::OnPostLogin(AController* Controller)
{
	Super::OnPostLogin(Controller);

	// 生成角色
	if (MatchState == MatchState::InProgress)
	{
		HandleSpawn(Controller);
	}
}

// 角色受到伤害
void AMeleeMode::HumanReceiveDamage(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
	float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser)
{
	if (DamagedCharacter == nullptr || DamagedController == nullptr || AttackerController == nullptr || DamageCauser == nullptr) return;

	AMeleePlayerState* DamagedState = Cast<AMeleePlayerState>(DamagedController->PlayerState);
	AMeleePlayerState* AttackerState = Cast<AMeleePlayerState>(AttackerController->PlayerState);

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
			if (MeleeGameState == nullptr) MeleeGameState = GetGameState<AMeleeGameState>();
			if (MeleeGameState)
			{
				MeleeGameState->AddTeamScore(AttackerState->Team);
			}

			// 增加攻击者连杀
			AttackerState->AddKillStreak();
		}

		// 击杀日志
		AddKillLog(AttackerState, DamageCauser, DamageType, DamagedState);

		// 增加受伤者死亡次数
		DamagedState->AddDeath();

		// 处理受伤者死亡流程
		DamagedCharacter->MulticastMeleeDead();
	}
}

// 角色重生
void AMeleeMode::HumanRespawn(ACharacter* Character, AController* Controller)
{
	if (Character && Controller)
	{
		Character->Destroy();

		SpawnHumanCharacter(Controller);
	}
}

void AMeleeMode::HandleChangeLobbyStatus()
{
	float Time = GetWorld()->GetTimeSeconds() - LevelStartTime - WarmupTime;
	int64 Minute = Time / 60 + 1;
	ChangeLobbyStatus(Minute);
}
