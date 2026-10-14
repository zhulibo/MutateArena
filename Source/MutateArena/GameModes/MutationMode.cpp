#include "MutationMode.h"

#include "DataRegistrySubsystem.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
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
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Equipments/Herbs/HerbRage.h"
#include "MutateArena/Equipments/Herbs/HerbRepelReceived.h"
#include "MutateArena/Equipments/Pickups/ItemSpawnPoint.h"
#include "MutateArena/Maps/Environment/EnvironmentAsset.h"
#include "MutateArena/Maps/Environment/EnvironmentManager.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/System/GameplayTags.h"

namespace MatchState
{
	const FName PostRound = TEXT("PostRound");
}

void AMutationMode::BeginPlay()
{
	Super::BeginPlay();

	if (UEOSSubsystem* EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>())
	{
		if (EOSSubsystem->GetLobbyMatchRound() > 0)
		{
			TotalRound = EOSSubsystem->GetLobbyMatchRound();
		}
	}

#if WITH_EDITOR
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

		// 使用反射获取PIE设置中的玩家数量，避免Runtime模块强依赖UnrealEd导致打包失败
		int32 PIEPlayerCount = 1;
		if (UClass* PlaySettingsClass = FindObject<UClass>(nullptr, TEXT("/Script/UnrealEd.LevelEditorPlaySettings")))
		{
			if (UObject* PlaySettings = PlaySettingsClass->GetDefaultObject())
			{
				if (FIntProperty* NumClientsProp = FindFProperty<FIntProperty>(PlaySettingsClass, TEXT("PlayNumberOfClients")))
				{
					PIEPlayerCount = NumClientsProp->GetPropertyValue_InContainer(PlaySettings);
				}
			}
		}

		if (PIEPlayerCount > 0)
		{
			WarmupTime *= PIEPlayerCount;
		}
	}
#endif

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
		TArray<ABasePlayerState*> PlayerStates;
		BaseGameState->GetPlayerStates(ETeam::Team1, PlayerStates);
		for (ABasePlayerState* BasePlayerState : PlayerStates)
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
	GetWorldTimerManager().ClearTimer(SpawnHerbRageTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnHerbRepelReceivedTimerHandle);

	RoundEndTime = GetWorld()->GetTimeSeconds();
	SetMatchState(MatchState::PostRound);

	HandleRoundHasEnded();
}

// 开启下一回合
void AMutationMode::StartNextRound()
{
#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			if (GetDefault<UDevSetting>()->bKeepInMap && MutationGameState->Team1PlayerStates.Num() + MutationGameState->Team2PlayerStates.Num() == 1)
			{
				return;
			}
		}
	}
#endif

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
	GetWorldTimerManager().ClearTimer(SpawnHerbRageTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnHerbRepelReceivedTimerHandle);

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
		TArray<AActor*> EquipmentsToDestroy = MutationGameState->AllEquipments; // 复制一份数组进行遍历，避免因为 Destroy() 触发 RemoveEquipment 导致遍历崩溃或遗漏
		for (AActor* Equipment : EquipmentsToDestroy)
		{
			if (IsValid(Equipment))
			{
				Equipment->Destroy();
			}
		}

		// 重置GameState中的回合专属状态与玩家阵营
		if (MutationGameState == nullptr) MutationGameState = GetGameState<AMutationGameState>();
		if (MutationGameState)
		{
			MutationGameState->ResetRoundDamageModifiers();
			MutationGameState->ClearAllTeamStates();
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
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::RandomMutate, MutateTime);

	// 对局开始时有1/10概率启用一个随机环境
	if (MutationGameState == nullptr) MutationGameState = GetGameState<AMutationGameState>();
	if (MutationGameState)
	{
		if (AEnvironmentManager* EnvManager = MutationGameState->EnvironmentManager)
		{
			if (FMath::RandRange(1, 10) == 1)
			{
				if (UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>())
				{
					TArray<FGameplayTag> AvailableEnvTags;
					AssetSubsystem->CachedEnvironmentAssets.GetKeys(AvailableEnvTags);

					if (AvailableEnvTags.Num() > 0)
					{
						int32 RandomIndex = FMath::RandRange(0, AvailableEnvTags.Num() - 1);
						EnvManager->ActivateEnvironment(AvailableEnvTags[RandomIndex]);
					}
				}
			}
		}
	}
}

// 对局结束
void AMutationMode::HandleRoundHasEnded()
{
	if (AMutationGameState* MutateGameState = GetGameState<AMutationGameState>())
	{
		if (AEnvironmentManager* EnvManager = MutateGameState->EnvironmentManager)
		{
			int32 ActiveEnvCount = EnvManager->ActiveEnvironments.Num();

			// 如果至少有一个环境是生效状态，则 1/3 概率结束其中一个
			if (ActiveEnvCount > 0 && FMath::RandRange(1, 3) == 1)
			{
				int32 RandomIndex = FMath::RandRange(0, ActiveEnvCount - 1);

				if (UEnvironmentAsset* TargetAsset = EnvManager->ActiveEnvironments[RandomIndex].EnvAsset)
				{
					EnvManager->DeactivateEnvironment(TargetAsset->EnvironmentTag);
				}
			}
		}
	}
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
void AMutationMode::RandomMutate()
{
	if (MutationGameState == nullptr) MutationGameState = GetGameState<AMutationGameState>();
	if (MutationGameState == nullptr) return;

	TArray<ABasePlayerState*> PlayerStates;
	BaseGameState->GetPlayerStates(ETeam::Team1, PlayerStates);

	int32 MutateNum;
	if (PlayerStates.Num() <= 1)
	{
		MutateNum = 0;
	}
	else if (PlayerStates.Num() >= 2 && PlayerStates.Num() <= 6)
	{
		MutateNum = 1;
	}
	else if (PlayerStates.Num() >= 7 && PlayerStates.Num() <= 14)
	{
		MutateNum = 2;
	}
	else if (PlayerStates.Num() >= 15 && PlayerStates.Num() <= 18)
	{
		MutateNum = 3;
	}
	else
	{
		MutateNum = 4;
	}

	for (int i = 0; i < MutateNum; ++i)
	{
		int32 RandomIndex = FMath::RandRange(0, PlayerStates.Num() - 1);

#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::PIE)
		{
			if (MutateNum == 1)
			{
				int32 MutateClientIndex = GetDefault<UDevSetting>()->MutateClientIndex;
				if (MutateClientIndex >= 0)
				{
					RandomIndex = MutateClientIndex;

					// 设置的玩家索引超出玩家数量，则使用最后一个玩家
					if (RandomIndex > PlayerStates.Num() - 1)
					{
						RandomIndex = PlayerStates.Num() - 1;
					}
				}
			}
		}
#endif

		if (ABasePlayerState* BasePlayerState = PlayerStates[RandomIndex])
		{
			// 初始突变体拥有最高等级
			if (BasePlayerState->GetAttributeSetBase())
			{
				BasePlayerState->GetAttributeSetBase()->SetCharacterLevel(3.f);
			}

			if (AController* Controller = Cast<AController>(BasePlayerState->GetOwner()))
			{
				Mutate(Controller->GetCharacter(), Controller, ESpawnMutantReason::RoundStartMutate);
			}

			// 初始突变体拥有满怒气
			if (AMutationPlayerState* MutationPlayerState = Cast<AMutationPlayerState>(BasePlayerState))
			{
				MutationPlayerState->AddRage(MutationPlayerState->RageLevel3);
			}
		}
	}
	// 突变后开始监视对局状态
	bWatchRoundState = true;

	// 定时生成补给箱
	GetWorldTimerManager().SetTimer(SpawnPickupTimerHandle, this, &ThisClass::SpawnPickups, 40.f, true, 35.f);
	// 定时生成草药
	GetWorldTimerManager().SetTimer(SpawnHerbRageTimerHandle, this, &ThisClass::SpawnHerbRages, FMath::FRandRange(10.f, 20.f));
	GetWorldTimerManager().SetTimer(SpawnHerbRepelReceivedTimerHandle, this, &ThisClass::SpawnHerbRepelReceived, 100.f);
}

void AMutationMode::HumanReceiveDamage(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
                                       float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser)
{
	if (DamagedCharacter == nullptr || DamagedController == nullptr) return;

	AMutationPlayerState* DamagedState = Cast<AMutationPlayerState>(DamagedController->PlayerState);
	if (DamagedState == nullptr) return;

	// 只有存在攻击者时，才尝试获取 AttackerState
	AMutationPlayerState* AttackerState = nullptr;
	if (AttackerController)
	{
		AttackerState = Cast<AMutationPlayerState>(AttackerController->PlayerState);
	}

	// 设置受伤者血量
	float TakenDamage = FMath::Clamp(Damage, 0.f, DamagedCharacter->GetHealth());
	float Health = DamagedCharacter->GetHealth() - TakenDamage;
	DamagedCharacter->SetHealth(Health);

	// 处理攻击者得分和怒气 (如果存在攻击者，且不是自己伤害自己)
	if (AttackerState && AttackerState != DamagedState) // 受到跌落伤害时，AttackerController和DamageCauser传的是自己
	{
		AttackerState->AddRage(Damage * 10);
		AttackerState->AddDamage(TakenDamage);
	}

	// 人类死亡判定
	if (DamagedCharacter->GetHealth() <= 0.f)
	{
		if (AttackerState && AttackerState != DamagedState)
		{
			AttackerState->AddKillStreak();
		}

		// 击杀日志
		AddKillLog(AttackerState, DamageCauser, DamageType, DamagedState);

		// 突变逻辑判定
		bool bMutateImmediately = false;
		if (const UDamageTypeBase* DamageTypeBase = Cast<UDamageTypeBase>(DamageType))
		{
			if (DamageTypeBase->DamageType == EDamageCauserType::MutantDamage)
			{
				bMutateImmediately = true;
				Mutate(DamagedCharacter, DamagedController, ESpawnMutantReason::Damaged);
			}
		}

		// 延迟重生 (酸雨致死、摔死等环境伤害通常走这里)
		if (!bMutateImmediately)
		{
			DamagedCharacter->MutationDead(true, ESpawnMutantReason::Fall); // 你可以加一个枚举专门表示 Environment 致死
		}
	}
}

// 突变体受到伤害
void AMutationMode::MutantReceiveDamage(AMutantCharacter* DamagedCharacter, ABaseController* DamagedController,
                                        float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser)
{
	if (MutationGameState == nullptr) MutationGameState = GetGameState<AMutationGameState>();
	if (MutationGameState == nullptr) return;

	if (DamagedCharacter == nullptr || DamagedController == nullptr) return;

	AMutationPlayerState* DamagedState = Cast<AMutationPlayerState>(DamagedController->PlayerState);
	if (DamagedState == nullptr) return;

	AMutationPlayerState* AttackerState = nullptr;
	if (AttackerController)
	{
		AttackerState = Cast<AMutationPlayerState>(AttackerController->PlayerState);
	}

	// 击退受伤者
	if (AProjectileBullet* ProjectileBullet = Cast<AProjectileBullet>(DamageCauser))
	{
		FVector ImpulseDirection = ProjectileBullet->GetActorForwardVector();
		float ForceMagnitude = ProjectileBullet->GetImpulse(Damage) * DamagedState->GetRepelReceivedMul();
		DamagedCharacter->ApplySuppressionForce(ImpulseDirection, ForceMagnitude);
	}

	// 设置受伤者血量
	float TakenDamage = Damage * MutationGameState->DamageMul * DamagedState->GetDamageReceivedMul();
	const UDamageTypeBase* DamageTypeBase = Cast<UDamageTypeBase>(DamageType);
	if (DamageTypeBase && DamageTypeBase->DamageType == EDamageCauserType::Melee)
	{
		TakenDamage *= MutationGameState->MeleeDamageMul;
	}
	TakenDamage = FMath::Clamp(TakenDamage, 0.f, DamagedCharacter->GetHealth());
	float Health = DamagedCharacter->GetHealth() - TakenDamage;
	DamagedCharacter->SetHealth(Health);

	// 增加受伤者的怒气值
	DamagedState->AddRage(TakenDamage);

	// 处理攻击者得分 (如果存在攻击者，且不是自己伤害自己)
	if (AttackerState && AttackerState != DamagedState) // 受到跌落伤害时，AttackerController和DamageCauser传的是自己
	{
		// 增加攻击者伤害分数
		AttackerState->AddDamage(TakenDamage);
	}

	// 突变体死亡
	if (DamagedCharacter->GetHealth() <= 0.f)
	{
		if (AttackerState && AttackerState != DamagedState)
		{
			// 增加攻击者连杀
			AttackerState->AddKillStreak();
		}

		// 击杀日志
		AddKillLog(AttackerState, DamageCauser, DamageType, DamagedState);

		// 处理突变体死亡流程
		bool bKilledByMelee = false;
		if (DamageTypeBase && DamageTypeBase->DamageType == EDamageCauserType::Melee)
		{
			bKilledByMelee = true;
			DamagedState->bKilledByMelee = true;
			MutationGameState->EndRoundIfAllBeKilledByMelee();
		}

		DamagedCharacter->MutantDead(bKilledByMelee);
	}
}

// 人类受到感染
void AMutationMode::GetInfect(AHumanCharacter* DamagedCharacter, ABaseController* DamagedController,
                              AMutantCharacter* AttackerCharacter, ABaseController* AttackerController)
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
	AttackerState->AddRage(2000.f);

	// 击杀日志
	AddKillLog(AttackerState, AttackerCharacter, GetDefault<UDamageTypeMutantInfect>(), DamagedState);

	// 人类感染后突变
	Mutate(DamagedCharacter, DamagedController, ESpawnMutantReason::Infected);
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
		HumanCharacter->MutationDead(false);
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
	
	ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(Controller->PlayerState);
	if (!BasePlayerState) return;

	// 获取角色类
	TSubclassOf<AMutantCharacter> CharacterClass;
	if (UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>())
	{
		if (UCharacterMutantAsset* CharacterMutantAsset = AssetSubsystem->GetCharacterMutantAssetByName(BasePlayerState->MutantCharacterName))
		{
			CharacterClass = CharacterMutantAsset->CharacterClass.LoadSynchronous();
		}
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
		Controller->SetControlRotation(MutantCharacter->GetActorRotation());
		Controller->ClientSetRotation(MutantCharacter->GetActorRotation());
	}
	// 瞬时切换角色，维持俯仰
	if (SpawnMutantReason == ESpawnMutantReason::RoundStartMutate
		|| SpawnMutantReason == ESpawnMutantReason::Infected
		|| SpawnMutantReason == ESpawnMutantReason::Damaged
		|| SpawnMutantReason == ESpawnMutantReason::SelectMutant
	)
	{
		Controller->SetControlRotation(ViewRotation);
		Controller->ClientSetRotation(ViewRotation);
	}
}

// 生成补给箱
void AMutationMode::SpawnPickups()
{
	// 寻找补给箱重生点
	if (SpawnPoints_Pickup.IsEmpty())
	{
		for (TActorIterator<AItemSpawnPoint> It(GetWorld()); It; ++It)
		{
			if (It->ItemType == ESpawnItemType::Pickup)
			{
				SpawnPoints_Pickup.Add(*It);
			}
		}
	}

	// 随机选3个重生点
	auto SelectedStartPoints = SpawnPoints_Pickup;
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
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
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
	if (SpawnPoints_HerbRage.IsEmpty())
	{
		for (TActorIterator<AItemSpawnPoint> It(GetWorld()); It; ++It)
		{
			if (It->ItemType == ESpawnItemType::HerbRage)
			{
				SpawnPoints_HerbRage.Add(*It);
			}
		}
	}

	// 随机选3个重生点
	auto SelectedStartPoints = SpawnPoints_HerbRage;
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
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
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
	if (SpawnPoints_HerbRepelReceived.IsEmpty())
	{
		for (TActorIterator<AItemSpawnPoint> It(GetWorld()); It; ++It)
		{
			if (It->ItemType == ESpawnItemType::HerbRepelReceived)
			{
				SpawnPoints_HerbRepelReceived.Add(*It);
			}
		}
	}

	// 随机选3个重生点
	auto SelectedStartPoints = SpawnPoints_HerbRepelReceived;
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
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
		GetWorld()->SpawnActor<AHerbRepelReceived>(
			HerbRepelReceivedClasses[FMath::RandRange(0, HerbRepelReceivedClasses.Num() - 1)],
			SelectedStartPoints[i]->GetActorLocation(),
			SelectedStartPoints[i]->GetActorRotation(),
			SpawnParams
		);
	}
}
