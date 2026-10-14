#include "BaseMode.h"

#include "DataRegistryId.h"
#include "DataRegistrySubsystem.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
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
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/DevSetting.h"
#include "Online/SchemaTypes.h"

#define LOCTEXT_NAMESPACE "ABaseMode"

ABaseMode::ABaseMode()
{
	bDelayedStart = true;
}

void ABaseMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 强制覆盖当前加载地图的 KillZ
	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->KillZ = -10000.0f;
		}
	}
}

void ABaseMode::BeginPlay()
{
	Super::BeginPlay();

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

void ABaseMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	// 游戏结束修改大厅状态
	ChangeLobbyStatus(0);
}

void ABaseMode::Logout(AController* Exiting)
{
	UE_LOG(LogTemp, Warning, TEXT("Logout ------------------------------------------"));

	if (Exiting && MatchState != MatchState::LeavingMap)
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

#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			if (GetDefault<UDevSetting>()->bKeepInMap) return;
		}
	}
#endif

	// 所有客户端退出后，再让服务端退出。
	if (MatchState == MatchState::LeavingMap && GetWorld()->GetNumPlayerControllers() == 2)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleLeavingMap ------------------------------------------"));
			PlayerController->ClientTravel(MapPaths::Menu, ETravelType::TRAVEL_Absolute);
		}
	}

	Super::Logout(Exiting);
}

void ABaseMode::EndMatch()
{
#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bUseMutationSettings)
		{
			if (GetDefault<UDevSetting>()->bKeepInMap) return;
		}
	}
#endif

	Super::EndMatch();
}

// 离开地图
void ABaseMode::HandleLeavingMap()
{
	Super::HandleLeavingMap();

	// 游戏结束服务端退出，或ABaseMode::Logout 中服务端退出失败，强制服务端退出。
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]()
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleLeavingMap ------------------------------------------"));
			PlayerController->ClientTravel(MapPaths::Menu, ETravelType::TRAVEL_Absolute);
		}
	});
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, 1.f, false);
}

// 生成人类角色
void ABaseMode::SpawnHumanCharacter(AController* Controller)
{
	if (Controller == nullptr) return;

	ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(Controller->PlayerState);
	if (!BasePlayerState) return;

	// 获取角色类
	TSubclassOf<AHumanCharacter> CharacterClass;
	if (UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>())
	{
		if (UCharacterHumanAsset* CharacterHumanAsset = AssetSubsystem->GetCharacterHumanAssetByName(BasePlayerState->HumanCharacterName))
		{
			CharacterClass = CharacterHumanAsset->CharacterClass.LoadSynchronous();
		}
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
				if (It->PlayerStartTag == TEXT("Team1"))
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
				if (It->PlayerStartTag == TEXT("Team2"))
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
	bool bIsMelee = false;

	// 提前获取 AssetSubsystem
	UAssetSubsystem* AssetSubsystem = nullptr;
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		AssetSubsystem = GameInstance->GetSubsystem<UAssetSubsystem>();
	}

	if (const UDamageTypeBase* DamageTypeBase = Cast<UDamageTypeBase>(DamageType))
	{
		switch (DamageTypeBase->DamageType)
		{
		case EDamageCauserType::Equipment:
			if (DamageCauser && DamageCauser->GetOwner())
			{
				if (AEquipment* CauserEquipment = Cast<AEquipment>(DamageCauser->GetOwner()))
				{
					// 使用 O(1) 的 AssetSubsystem 获取武器名称
					if (AssetSubsystem)
					{
						if (UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(CauserEquipment->EquipmentName))
						{
							CauserName = Asset->ShowName;
						}
					}
				}
			}
			break;
		case EDamageCauserType::Melee:
			if (AEquipment* CauserEquipment = Cast<AEquipment>(DamageCauser))
			{
				// 使用 O(1) 的 AssetSubsystem 获取近战武器名称
				if (AssetSubsystem)
				{
					if (UEquipmentAsset* Asset = AssetSubsystem->GetEquipmentAssetByName(CauserEquipment->EquipmentName))
					{
						CauserName = Asset->ShowName;
						bIsMelee = true;
					}
				}
			}
			break;
		case EDamageCauserType::MutantInfect:
			CauserName = LOCTEXT("Infect", "Infect");
			break;
		case EDamageCauserType::MutantDamage:
			CauserName = LOCTEXT("Damage", "Damage");
			break;
		case EDamageCauserType::Bleed:
			CauserName = LOCTEXT("Bleed", "Bleed");
			break;
		case EDamageCauserType::AcidRain:
			CauserName = LOCTEXT("AcidRain", "Acid Rain");
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

		if (bIsMelee && BaseGameState->ActorHasTag(Tags::GameStateMutation))
		{
			FText AttackerName = AttackerState ? FText::FromString(ULibraryCommon::ObfuscateName(AttackerState->GetPlayerName(), this)) : FText::GetEmpty();
			FText DamagedName = DamagedState ? FText::FromString(ULibraryCommon::ObfuscateName(DamagedState->GetPlayerName(), this)) : FText::GetEmpty();

			BaseGameState->MulticastMeleeLog(AttackerName, CauserName, DamagedName);
		}
	}
}

// 修改大厅状态
void ABaseMode::ChangeLobbyStatus(int64 Status)
{
	if (UEOSSubsystem* EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>())
	{
		EOSSubsystem->ModifyLobbyAttr(TMap<UE::Online::FSchemaAttributeId, UE::Online::FSchemaVariant>{
			{LobbyAttrs::Status, Status},
		});
	}
}

#undef LOCTEXT_NAMESPACE
