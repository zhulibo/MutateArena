#include "BaseMode.h"

#include "DataRegistryId.h"
#include "DataRegistrySubsystem.h"
#include "EngineUtils.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Data/DamageTypeFall.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/UI/TextChat/TextChat.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "GameFramework/PlayerStart.h"
#include "MutateArena/System/DevSetting.h"
#include "Online/SchemaTypes.h"

#define LOCTEXT_NAMESPACE "ABaseMode"

ABaseMode::ABaseMode()
{
	bDelayedStart = true;
}

void ABaseMode::BeginPlay()
{
	Super::BeginPlay();

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	
	LevelStartTime = GetWorld()->GetTimeSeconds();
}

void ABaseMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ABaseMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("OnPostLogin ------------------------------------------"));

	if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState == nullptr) return;

	if (ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(NewPlayer->PlayerState))
	{
		BaseGameState->MulticastSendMsg(EMsgType::Join, BasePlayerState->Team, BasePlayerState->GetPlayerName());
	}
}

void ABaseMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	UE_LOG(LogTemp, Warning, TEXT("Logout ------------------------------------------"));

	if (MatchState != MatchState::LeavingMap)
	{
		if (ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(Exiting->PlayerState))
		{
			if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
			if (BaseGameState)
			{
				BaseGameState->MulticastSendMsg(EMsgType::Left, BasePlayerState->Team, BasePlayerState->GetPlayerName());
			}
		}
	}

	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			if (GetDefault<UDevSetting>()->bKeepInMap) return;
		}
	}
	// 所有客户端退出后，再让服务端退出。
	if (MatchState == MatchState::LeavingMap && GetWorld()->GetNumPlayerControllers() == 2)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleLeavingMap ------------------------------------------"));
			PlayerController->ClientTravel(MAP_MENU, ETravelType::TRAVEL_Absolute);
		}
	}
}

// 离开地图
void ABaseMode::HandleLeavingMap()
{
	Super::HandleLeavingMap();
	
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			if (GetDefault<UDevSetting>()->bKeepInMap) return;
		}
	}
	// 游戏结束服务端退出，或ABaseMode::Logout 中服务端退出失败，强制服务端退出。
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]() {
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleLeavingMap ------------------------------------------"));
			PlayerController->ClientTravel(MAP_MENU, ETravelType::TRAVEL_Absolute);
		}
	});
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 1.f, false);
}

// 生成人类角色
void ABaseMode::SpawnHumanCharacter(AController* Controller)
{
	if (Controller == nullptr) return;

	// 获取角色名字
	FString CharacterName;
	ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(Controller->PlayerState);
	if (BasePlayerState)
	{
		CharacterName = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(BasePlayerState->HumanCharacterName));
	}

	// 获取角色类
	TSubclassOf<AHumanCharacter> CharacterClass;
	FDataRegistryId DataRegistryId(DR_HUMAN_CHARACTER_MAIN, FName(CharacterName));
	if (const FHumanCharacterMain* HumanCharacterMain = UDataRegistrySubsystem::Get()->GetCachedItem<FHumanCharacterMain>(DataRegistryId))
	{
		CharacterClass = UDataAssetManager::Get().GetSubclass(HumanCharacterMain->HumanCharacterClass);
	}
	if (CharacterClass == nullptr) return;

	// 获取出生点
	AActor* StartSpot = FindCharacterPlayerStart(BasePlayerState->Team);
	if (StartSpot == nullptr) return;

	// 生成角色
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AHumanCharacter* HumanCharacter = GetWorld()->SpawnActor<AHumanCharacter>(
		CharacterClass,
		StartSpot->GetActorLocation(),
		StartSpot->GetActorRotation(),
		SpawnParams
	);

	// 搞搞Controller
	Controller->Possess(HumanCharacter);
	// 重置俯仰
	Controller->ClientSetRotation(HumanCharacter->GetActorRotation());
}

// 寻找角色出生点
AActor* ABaseMode::FindCharacterPlayerStart(ETeam Team)
{
	APlayerStart* PlayerStart = nullptr;

	if (Team == ETeam::Team1)
	{
		if (Team1PlayerStarts.IsEmpty())
		{
			for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
			{
				if (It->PlayerStartTag == "Team1")
				{
					Team1PlayerStarts.Add(*It);
				}
			}
		}

		PlayerStart = Team1PlayerStarts[FMath::RandRange(0, Team1PlayerStarts.Num() - 1)];
	}
	else if (Team == ETeam::Team2)
	{
		if (Team2PlayerStarts.IsEmpty())
		{
			for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
			{
				if (It->PlayerStartTag == "Team2")
				{
					Team2PlayerStarts.Add(*It);
				}
			}
		}

		PlayerStart = Team2PlayerStarts[FMath::RandRange(0, Team2PlayerStarts.Num() - 1)];
	}

	return PlayerStart;
}

// 分配队伍
void ABaseMode::AssignTeam(AController* Controller, ETeam Team)
{
	if (Controller == nullptr) return;

	if (BaseGameState == nullptr) BaseGameState = GetGameState<ABaseGameState>();
	if (BaseGameState == nullptr) return;

	if (ABasePlayerState* BasePlayerState = Controller->GetPlayerState<ABasePlayerState>())
	{
		BaseGameState->AddToPlayerStates(BasePlayerState, Team);
		BasePlayerState->SetTeam(Team);

		BaseGameState->RemoveFromPlayerStates(BasePlayerState, Team == ETeam::Team1 ? ETeam::Team2 : ETeam::Team1);
	}
}

// 击杀日志
void ABaseMode::AddKillLog(ABasePlayerState* AttackerState, AActor* DamageCauser, const UDamageType* DamageType, ABasePlayerState* DamagedState)
{
	FText CauserName = FText::FromString(TEXT("-1"));

	if (const UDamageTypeBase* DamageTypeBase = Cast<UDamageTypeBase>(DamageType))
	{
		switch (DamageTypeBase->DamageType)
		{
		case EDamageCauserType::Equipment:
			if (AEquipment* CauserEquipment = Cast<AEquipment>(DamageCauser->GetOwner()))
			{
				FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(CauserEquipment->EquipmentName));
				FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
				if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
				{
					FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, EquipmentMain->ShowName, CauserName);
				}
			}
			break;
		case EDamageCauserType::Melee:
			if (AEquipment* CauserEquipment = Cast<AEquipment>(DamageCauser))
			{
				FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(CauserEquipment->EquipmentName));
				FDataRegistryId DataRegistryId(DR_EQUIPMENT_MAIN, FName(EnumValue));
				if (const FEquipmentMain* EquipmentMain = UDataRegistrySubsystem::Get()->GetCachedItem<FEquipmentMain>(DataRegistryId))
				{
					FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, EquipmentMain->ShowName, CauserName);
				}
			}
			break;
		case EDamageCauserType::MutantInfect:
			CauserName = LOCTEXT("Infect", "Infect");
			break;
		case EDamageCauserType::MutantDamage:
			CauserName = LOCTEXT("Damage", "Damage");
			break;
		case EDamageCauserType::Fall:
			CauserName = LOCTEXT("Fall", "Fall");
			break;
		}
	}

	if (BaseGameState == nullptr) BaseGameState = GetGameState<ABaseGameState>();
	if (BaseGameState)
	{
		BaseGameState->MulticastAddKillLog(AttackerState, CauserName, DamagedState);
	}
}

// 修改大厅状态
void ABaseMode::ChangeLobbyStatus(int64 Status)
{
	if (EOSSubsystem && Status > 0)
	{
		EOSSubsystem->ModifyLobbyAttr(TMap<UE::Online::FSchemaAttributeId, UE::Online::FSchemaVariant>{
			{LOBBY_STATUS, Status},
		});
	}
}

#undef LOCTEXT_NAMESPACE
