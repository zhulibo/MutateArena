#include "MutationMode.h"

#include "DataRegistrySubsystem.h"
#include "EngineUtils.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/PlayerStates/MutationPlayerState.h"
#include "MutateArena/GameStates/MutationGameState.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Data/DamageTypeMutantInfect.h"
#include "MutateArena/Equipments/Data/DamageTypeMutantDamage.h"
#include "MutateArena/Equipments/Pickups/Pickup.h"
#include "MutateArena/Equipments/Projectiles/ProjectileBullet.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "GameFramework/PlayerStart.h"
#include "MutateArena/Equipments/Herbs/HerbRage.h"
#include "MutateArena/Equipments/Herbs/HerbRepelReceived.h"

namespace MatchState
{
	const FName PostRound = TEXT("PostRound");
}

void AMutationMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			TotalRound = GetDefault<UDevSetting>()->TotalRound;
			WarmupTime = GetDefault<UDevSetting>()->WarmupTime;
			RoundTime = GetDefault<UDevSetting>()->RoundTime;
			MutateTime = GetDefault<UDevSetting>()->MutateTime;
			PostRoundTime = GetDefault<UDevSetting>()->PostRoundTime;
			CooldownTime = GetDefault<UDevSetting>()->CooldownTime;
		}
	}

	MutationGameState = GetGameState<AMutationGameState>();
}

void AMutationMode::Tick(float DeltaSeconds)
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
	// 对局中（一场比赛有多局）
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = RoundStartTime + RoundTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f)
		{
			EndRound();
		}
	}
	// 对局后
	else if (MatchState == MatchState::PostRound)
	{
		CountdownTime = RoundEndTime + PostRoundTime - GetWorld()->GetTimeSeconds();
		if (CountdownTime <= 0.f)
		{
			if (CurRound < TotalRound)
			{
				StartNextRound();
			}
			else
			{
				EndMatch();
			}
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

bool AMutationMode::IsMatchInProgress() const
{
	if (GetMatchState() == MatchState::PostRound)
	{
		return true;
	}

	return Super::IsMatchInProgress();
}

// 开始游戏（开始第一回合）
void AMutationMode::StartMatch()
{
	RoundStartTime = GetWorld()->GetTimeSeconds();

	Super::StartMatch();
}

// 结束回合
void AMutationMode::EndRound()
{
	// 存活
	if (MutationGameState == nullptr) MutationGameState = GetGameState<AMutationGameState>();
	if (MutationGameState)
	{
		TArray<ABasePlayerState*> BasePlayerStates = MutationGameState->GetPlayerStates(ETeam::Team1);
		for (ABasePlayerState* BasePlayerState : BasePlayerStates)
		{
			if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(BasePlayerState->GetPawn()))
			{
				if (!BaseCharacter->bIsDead)
				{
					BasePlayerState->AddSurvive(1);
				}
			}
		}
	}

	// 对局时间结束时结束监视对局状态
	bWatchRoundState = false;

	// 清除生成补给箱定时器
	GetWorldTimerManager().ClearTimer(SpawnPickupTimerHandle);

	RoundEndTime = GetWorld()->GetTimeSeconds();
	SetMatchState(MatchState::PostRound);
}

// 开启下一回合
void AMutationMode::StartNextRound()
{
	CurRound++;
	RoundStartTime = GetWorld()->GetTimeSeconds();
	SetMatchState(MatchState::InProgress);

	ChangeLobbyStatus(CurRound);
}

// 结束比赛
void AMutationMode::EndMatch()
{
	if (bIsEndingMatch) return;
	bIsEndingMatch = true;

	// 比赛结束时结束监视对局和比赛状态
	bWatchRoundState = false;
	bWatchMatchState = false;

	// 清除生成补给箱定时器
	GetWorldTimerManager().ClearTimer(SpawnPickupTimerHandle);

	MatchEndTime = GetWorld()->GetTimeSeconds();

	Super::EndMatch();
}

// MatchState变化时通知所有Controller
void AMutationMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMutationController* MutationController = Cast<AMutationController>(*It))
		{
			MutationController->OnMatchStateSet(MatchState, CurRound);
		}
	}
}

// 对局开始
void AMutationMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// 重新开始回合时，重置状态
	if (CurRound > 1)
	{
		// 重置关卡
		ResetLevel();

		// 销毁所有装备
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			if (AEquipment* Equipment = Cast<AEquipment>(*It))
			{
				Equipment->Destroy();
			}
		}
	}

	// 生成角色
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMutationController* MutationController = Cast<AMutationController>(*It))
		{
			MutationController->SetPlayerPlay();

			AssignTeam(MutationController, ETeam::Team1);

			SpawnHumanCharacter(MutationController);
		}
	}

	// 第一回合角色生成后开始监视比赛状态
	if (CurRound == 1)
	{
		bWatchMatchState = true;
	}

	// 倒计时结束，选择人类突变
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::RoundStartMutate, MutateTime);
}

// 中途加入
void AMutationMode::OnPostLogin(AController* Controller)
{
	Super::OnPostLogin(Controller);
	
	if (MatchState == MatchState::InProgress || MatchState == MatchState::PostRound || MatchState == MatchState::WaitingPostMatch)
	{
		// 突变倒计时未结束生成人类
		if (GetWorld()->GetTimeSeconds() < RoundStartTime + MutateTime)
		{
			AssignTeam(Controller, ETeam::Team1);
			SpawnHumanCharacter(Controller);
		}
		// 突变倒计时已结束生成突变体
		else
		{
			AssignTeam(Controller, ETeam::Team2);
			SpawnMutantCharacter(Controller, ESpawnMutantReason::FirstJoinGame);
		}
	}
}

// 开局随机选择人类突变
void AMutationMode::RoundStartMutate()
{
	if (MutationGameState == nullptr) MutationGameState = GetGameState<AMutationGameState>();
	if (MutationGameState == nullptr) return;

	TArray<ABasePlayerState*> Team1 = MutationGameState->GetPlayerStates(ETeam::Team1);

	int32 MutateNum;
	if (Team1.Num() <= 6)
	{
		MutateNum = 1;
	}
	else if (Team1.Num() >= 7 && Team1.Num() <= 14)
	{
		MutateNum = 2;
	}
	else if (Team1.Num() >= 15 && Team1.Num() <= 18)
	{
		MutateNum = 3;
	}
	else
	{
		MutateNum = 4;
	}

	for (int i = 0; i < MutateNum; ++i)
	{
		int32 RandomIndex = FMath::RandRange(0, Team1.Num() - 1);
		
		if (GetWorld()->WorldType == EWorldType::PIE)
		{
			RandomIndex = GetDefault<UDevSetting>()->MutateClientIndex;

			if (RandomIndex > Team1.Num() - 1)
			{
				RandomIndex = Team1.Num() - 1;
			}
		}

		if (ABasePlayerState* BasePlayerState = Team1[RandomIndex])
		{
			// 初始突变体拥有最高等级
			if (BasePlayerState->GetAttributeSetBase())
			{
				BasePlayerState->GetAttributeSetBase()->SetCharacterLevel(3.f);
			}

			if (AController* Controller = Cast<AController>(BasePlayerState->GetOwner()))
			{
				Mutate(Controller->GetCharacter(), Controller, ESpawnMutantReason::RoundStart);
			}
		}
	}
	// 突变后开始监视对局状态
	bWatchRoundState = true;

	// 定时生成补给箱
	GetWorldTimerManager().SetTimer(SpawnPickupTimerHandle, this, &ThisClass::SpawnPickups, 40.f, true, 35.f);
	// 定时生成草药
	GetWorldTimerManager().SetTimer(SpawnHerbRageTimerHandle, this, &ThisClass::SpawnHerbRages, FMath::FRandRange(10.f, 20.f));
	GetWorldTimerManager().SetTimer(SpawnHerbRepelReceivedTimerHandle, this, &ThisClass::SpawnHerbRepelReceived, 10.f);
}

// 人类受到伤害
void AMutationMode::HumanReceiveDamage(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
	float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser)
{
	if (DamagedCharacter == nullptr || DamagedController == nullptr || AttackerController == nullptr || DamageCauser == nullptr) return;

	AMutationPlayerState* DamagedState = Cast<AMutationPlayerState>(DamagedController->PlayerState);
	AMutationPlayerState* AttackerState = Cast<AMutationPlayerState>(AttackerController->PlayerState);

	if (DamagedState == nullptr || AttackerState == nullptr) return;

	// 设置受伤者血量
	float TakenDamage = FMath::Clamp(Damage, 0.f, DamagedCharacter->GetHealth());
	float Health = DamagedCharacter->GetHealth() - TakenDamage;
	DamagedCharacter->SetHealth(Health);
	DamagedCharacter->MulticastSetHealth(Health, AttackerController);

	if (AttackerState != DamagedState) // 受到跌落伤害时，AttackerController和DamageCauser传的是自己
	{
		// 增加攻击者怒气值
		AttackerState->SetRage(AttackerState->Rage + Damage * 10);
	}

	// 人类死亡
	if (DamagedCharacter->GetHealth() <= 0.f)
	{
		if (AttackerState != DamagedState)
		{
			// 增加攻击者连杀
			AttackerState->AddKillStreak();
		}

		// 击杀日志
		AddKillLog(AttackerState, DamageCauser, DamageType, DamagedState);

		// 人类因突变体伤害血量为0立刻突变
		bool bMutateImmediately = false;
		if (const UDamageTypeBase* DamageTypeBase = Cast<UDamageTypeBase>(DamageType))
		{
			if (DamageTypeBase->DamageType == EDamageCauserType::MutantDamage)
			{
				bMutateImmediately = true;

				Mutate(DamagedCharacter, DamagedController, ESpawnMutantReason::MutantDamage);
			}
		}

		// 摔死延迟重生
		if (!bMutateImmediately)
		{
			DamagedCharacter->MulticastMutationDead(true, ESpawnMutantReason::Fall);
		}
	}
}

// 人类受到感染
void AMutationMode::GetInfect(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
	AMutantCharacter* AttackerCharacter, ABaseController* AttackerController, EMutantState MutantState)
{
	if (DamagedCharacter == nullptr || DamagedController == nullptr || AttackerController == nullptr) return;

	AMutationPlayerState* DamagedState = Cast<AMutationPlayerState>(DamagedController->PlayerState);
	AMutationPlayerState* AttackerState = Cast<AMutationPlayerState>(AttackerController->PlayerState);

	if (DamagedState == nullptr || AttackerState == nullptr) return;

	// 增加攻击者感染数
	AttackerState->AddInfect(1);
	// 增加攻击者连杀
	AttackerState->AddKillStreak();
	// 增加攻击者怒气值
	AttackerState->SetRage(AttackerState->Rage + 2000.f);

	// 击杀日志
	AddKillLog(AttackerState, AttackerCharacter, GetDefault<UDamageTypeMutantInfect>(), DamagedState);

	// 人类感染后突变
	Mutate(DamagedCharacter, DamagedController, ESpawnMutantReason::MutantInfect);
}

// 人类突变为突变体
void AMutationMode::Mutate(ACharacter* Character, AController* Controller, ESpawnMutantReason SpawnMutantReason)
{
	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(Character))
	{
		// 记录当前角色的位置和旋转
		FVector Location = HumanCharacter->GetActorLocation();
		FRotator ActorRotation = Character->GetActorRotation();
		FRotator ViewRotation = Character->GetViewRotation();

		// 人类死亡
		HumanCharacter->MulticastMutationDead(false);
		HumanCharacter->Destroy();

		// 重生为突变体
		AssignTeam(Controller, ETeam::Team2);
		SpawnMutantCharacter(Controller, SpawnMutantReason, Location, ActorRotation, ViewRotation);
	}
}

// 突变体选择角色重生
void AMutationMode::SelectMutant(ACharacter* Character, AController* Controller)
{
	if (Character && Controller)
	{
		// 记录当前角色的位置和旋转
		FVector Location = Character->GetActorLocation();
		FRotator ActorRotation = Character->GetActorRotation();
		FRotator ViewRotation = Character->GetViewRotation();

		// 销毁旧角色
		Character->Destroy();

		// 生成新角色
		SpawnMutantCharacter(Controller, ESpawnMutantReason::SelectMutant, Location, ActorRotation, ViewRotation);
	}
}

// 突变体受到伤害
void AMutationMode::MutantReceiveDamage(AMutantCharacter* DamagedCharacter, ABaseController* DamagedController,
	float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser)
{
	if (MutationGameState == nullptr) MutationGameState = GetGameState<AMutationGameState>();
	if (MutationGameState == nullptr) return;

	if (DamagedCharacter == nullptr || DamagedController == nullptr || AttackerController == nullptr || DamageCauser == nullptr) return;

	AMutationPlayerState* DamagedState = Cast<AMutationPlayerState>(DamagedController->PlayerState);
	AMutationPlayerState* AttackerState = Cast<AMutationPlayerState>(AttackerController->PlayerState);

	if (DamagedState == nullptr || AttackerState == nullptr) return;

	// 击退受伤者
	if (AProjectileBullet* ProjectileBullet = Cast<AProjectileBullet>(DamageCauser))
	{
		FVector ImpulseVector = ProjectileBullet->GetActorForwardVector();
		ImpulseVector.Z = 0.f;
		DamagedCharacter->MulticastRepel(ImpulseVector * ProjectileBullet->GetImpulse(Damage) * DamagedState->GetRepelReceivedMul());
	}

	// 设置受伤者血量
	float TakenDamage = Damage * MutationGameState->DamageMul * DamagedState->GetDamageReceivedMul();
	TakenDamage = FMath::Clamp(TakenDamage, 0.f, DamagedCharacter->GetHealth());
	float Health = DamagedCharacter->GetHealth() - TakenDamage;
	DamagedCharacter->SetHealth(Health);
	DamagedCharacter->MulticastSetHealth(Health, AttackerController);

	// 增加怒气值
	DamagedState->SetRage(DamagedState->Rage + TakenDamage);

	// 增加攻击者伤害分数
	if (AttackerState != DamagedState) // 受到跌落伤害时，AttackerController和DamageCauser传的是自己
	{
		AttackerState->AddDamage(TakenDamage);
	}

	// 突变体死亡
	if (DamagedCharacter->GetHealth() <= 0.f)
	{
		if (AttackerState != DamagedState)
		{
			// 增加攻击者连杀
			AttackerState->AddKillStreak();
		}

		// 击杀日志
		AddKillLog(AttackerState, DamageCauser, DamageType, DamagedState);

		// 处理突变体死亡流程
		bool bKilledByMelee = false;
		if (const UDamageTypeBase* DamageTypeBase = Cast<UDamageTypeBase>(DamageType))
		{
			if (DamageTypeBase->DamageType == EDamageCauserType::Melee)
			{
				bKilledByMelee = true;
				DamagedState->bKilledByMelee = true;
				MutationGameState->EndRoundIfAllBeKilledByMelee();
			}
		}
		DamagedCharacter->MulticastDead(bKilledByMelee);
	}
}

// 突变体重生
void AMutationMode::MutantRespawn(ACharacter* Character, ABaseController* BaseController, bool bKilledByMelee)
{
	if (Character == nullptr || BaseController == nullptr) return;

	if (!bKilledByMelee)
	{
		Character->Destroy();
		SpawnMutantCharacter(BaseController, ESpawnMutantReason::Respawn);
	}
	// 被刀死不能复活，进入观察视角
	else
	{
		BaseController->SetPlayerSpectate();
	}
}

// 生成突变体角色
void AMutationMode::SpawnMutantCharacter(AController* Controller, ESpawnMutantReason SpawnMutantReason,
	FVector Location, FRotator ActorRotation, FRotator ViewRotation)
{
	if (Controller == nullptr) return;

	// 获取角色名字
	FString CharacterName;
	if (ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(Controller->PlayerState))
	{
		CharacterName = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(BasePlayerState->MutantCharacterName));
	}

	// 获取角色类
	TSubclassOf<AMutantCharacter> CharacterClass;
	FDataRegistryId DataRegistryId(DR_MUTANT_CHARACTER_MAIN, FName(CharacterName));
	if (const FMutantCharacterMain* MutantCharacterMain = UDataRegistrySubsystem::Get()->GetCachedItem<FMutantCharacterMain>(DataRegistryId))
	{
		CharacterClass = UDataAssetManager::Get().GetSubclass(MutantCharacterMain->MutantCharacterClass);
	}
	if (CharacterClass == nullptr) return;

	// 如果是突变体重生，或摔死的，使用出生点的位置和旋转
	if (SpawnMutantReason == ESpawnMutantReason::Respawn || SpawnMutantReason == ESpawnMutantReason::Fall)
	{
		if (AActor* StartSpot = FindCharacterPlayerStart(ETeam::Team2))
		{
			Location = StartSpot->GetActorLocation();
			ActorRotation = StartSpot->GetActorRotation();
		}
	}

	// 生成角色
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AMutantCharacter* MutantCharacter = GetWorld()->SpawnActor<AMutantCharacter>(
		CharacterClass,
		Location,
		ActorRotation,
		SpawnParams
	);

	MutantCharacter->SpawnMutantReason = SpawnMutantReason;

	Controller->Possess(MutantCharacter);

	// 如果是突变体重生，重置俯仰
	if (SpawnMutantReason == ESpawnMutantReason::Respawn)
	{
		Controller->ClientSetRotation(MutantCharacter->GetActorRotation());
	}
}

// 生成补给箱
void AMutationMode::SpawnPickups()
{
	// 寻找补给箱重生点
	if (PickupStartPoints.IsEmpty())
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == TEXT("Pickup"))
			{
				PickupStartPoints.Add(*It);
			}
		}
	}

	// 随机选3个重生点
	auto SelectedStartPoints = PickupStartPoints;
	while (SelectedStartPoints.Num() > 3)
	{
		int32 RandomIndex = FMath::RandRange(0, SelectedStartPoints.Num() - 1);
		SelectedStartPoints.RemoveAt(RandomIndex);
	}

	for (int i = 0; i < SelectedStartPoints.Num(); ++i)
	{
		if (SelectedStartPoints[i] == nullptr) continue;
		// 生成
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		GetWorld()->SpawnActor<APickup>(
			PickupClasses[FMath::RandRange(0, PickupClasses.Num() - 1)],
			SelectedStartPoints[i]->GetActorLocation(),
			SelectedStartPoints[i]->GetActorRotation(),
			SpawnParams
		);
	}

	// 播放补给箱抵达音效
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMutationController* MutationController = Cast<AMutationController>(*It))
		{
			MutationController->MulticastPlaySpawnPickupSound();
		}
	}
}

// 生成加怒气草药
void AMutationMode::SpawnHerbRages()
{
	// 寻找重生点
	if (HerbRageStartPoints.IsEmpty())
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == TEXT("HerbRage"))
			{
				HerbRageStartPoints.Add(*It);
			}
		}
	}

	// 随机选3个重生点
	auto SelectedStartPoints = HerbRageStartPoints;
	while (SelectedStartPoints.Num() > 3)
	{
		int32 RandomIndex = FMath::RandRange(0, SelectedStartPoints.Num() - 1);
		SelectedStartPoints.RemoveAt(RandomIndex);
	}

	for (int i = 0; i < SelectedStartPoints.Num(); ++i)
	{
		if (SelectedStartPoints[i] == nullptr) continue;
		// 生成
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		GetWorld()->SpawnActor<AHerbRage>(
			HerbRageClasses[FMath::RandRange(0, HerbRageClasses.Num() - 1)],
			SelectedStartPoints[i]->GetActorLocation(),
			SelectedStartPoints[i]->GetActorRotation(),
			SpawnParams
		);
	}
}

// 生成抗击退草药
void AMutationMode::SpawnHerbRepelReceived()
{
	// 寻找重生点
	if (HerbRepelReceivedStartPoints.IsEmpty())
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == TEXT("HerbRepelReceived"))
			{
				HerbRepelReceivedStartPoints.Add(*It);
			}
		}
	}

	// 随机选3个重生点
	auto SelectedStartPoints = HerbRepelReceivedStartPoints;
	while (SelectedStartPoints.Num() > 3)
	{
		int32 RandomIndex = FMath::RandRange(0, SelectedStartPoints.Num() - 1);
		SelectedStartPoints.RemoveAt(RandomIndex);
	}

	for (int i = 0; i < SelectedStartPoints.Num(); ++i)
	{
		if (SelectedStartPoints[i] == nullptr) continue;
		// 生成
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		GetWorld()->SpawnActor<AHerbRepelReceived>(
			HerbRepelReceivedClasses[FMath::RandRange(0, HerbRepelReceivedClasses.Num() - 1)],
			SelectedStartPoints[i]->GetActorLocation(),
			SelectedStartPoints[i]->GetActorRotation(),
			SpawnParams
		);
	}
}
