#include "BasePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TeamType.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Characters/Components/OverheadWidget.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/MutateArena.h"

ABasePlayerState::ABasePlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UMAAbilitySystemComponent>(TEXT("BaseAbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSetBase = CreateDefaultSubobject<UAttributeSetBase>(TEXT("BaseAttributeSet"));

	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		HumanCharacterName = DefaultConfig->HumanCharacterName;
		MutantCharacterName = DefaultConfig->MutantCharacterName;
	}

	Team = ETeam::NoTeam;
}

void ABasePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, HumanCharacterName);
	DOREPLIFETIME(ThisClass, MutantCharacterName);
	DOREPLIFETIME(ThisClass, AccountIdRepl);
	DOREPLIFETIME(ThisClass, Team);
	DOREPLIFETIME(ThisClass, Damage);
	DOREPLIFETIME(ThisClass, Defeat);
	DOREPLIFETIME(ThisClass, KillStreak);
}

void ABasePlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		UStorageSubsystem* StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheLoadout)
		{
			ServerSetHumanCharacterName(StorageSubsystem->CacheLoadout->HumanCharacterName);
			ServerSetMutantCharacterName(StorageSubsystem->CacheLoadout->MutantCharacterName);
		}

		// 向所有玩家分享AccountId
		if (UEOSSubsystem* EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>())
		{
			if (EOSSubsystem->GetAccountId().IsValid())
			{
				// ServerSetAccountId(FUniqueNetIdRepl(EOSSubsystem->GetAccountId()));
			}											
		}
	}

	InitData();
}

void ABasePlayerState::Reset()
{
	Super::Reset();

	InitData();
}

void ABasePlayerState::Destroyed()
{
	// 移除PlayerState
	if (HasAuthority())
	{
		if (ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>())
		{
			BaseGameState->RemoveFromPlayerStates(this, Team);
		}
	}

	Super::Destroyed();
}

// 初始化数据
void ABasePlayerState::InitData()
{
}

UAbilitySystemComponent* ABasePlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSetBase* ABasePlayerState::GetAttributeSetBase()
{
	return AttributeSetBase;
}

float ABasePlayerState::GetMaxHealth()
{
	return AttributeSetBase ? AttributeSetBase->GetMaxHealth() : 0.f;
}

float ABasePlayerState::GetHealth()
{
	return AttributeSetBase ? AttributeSetBase->GetHealth() : 0.f;
}

float ABasePlayerState::GetDamageReceivedMul()
{
	return AttributeSetBase ? AttributeSetBase->GetDamageReceivedMul() : 0.f;
}

float ABasePlayerState::GetRepelReceivedMul()
{
	return AttributeSetBase ? AttributeSetBase->GetRepelReceivedMul() : 0.f;
}

float ABasePlayerState::GetCharacterLevel()
{
	return AttributeSetBase ? AttributeSetBase->GetCharacterLevel() : 0.f;
}

float ABasePlayerState::GetJumpZVelocity()
{
	return AttributeSetBase ? AttributeSetBase->GetJumpZVelocity() : 0.f;
}

void ABasePlayerState::SetTeam(ETeam TempTeam)
{
	// UE_LOG(LogTemp, Warning, TEXT("SetTeam ------------------------------------------"));

	Team = TempTeam;

	BaseCharacter = nullptr;
}

void ABasePlayerState::OnRep_Team()
{
	// UE_LOG(LogTemp, Warning, TEXT("OnRep_Team ------------------------------------------"));

	BaseCharacter = nullptr;

	BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BaseCharacter)
	{
		BaseCharacter->bHasSetMeshCollisionType = false;
	}

	// InitOverheadWidget依赖于Team，OnRep_Team后主动调一下InitHUD。
	InitOverheadWidget();
}

void ABasePlayerState::InitOverheadWidget()
{
	if (BaseCharacter == nullptr) BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BaseCharacter)
	{
		if (!BaseCharacter->IsLocallyControlled())
		{
			if (UWidgetComponent* OverheadWidget = BaseCharacter->OverheadWidget)
			{
				if (UOverheadWidget* OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject()))
				{
					if (IsValid(OverheadWidgetClass))
					{
						OverheadWidgetClass->InitOverheadWidget();
					}
				}
			}
		}
		// 本地玩家队伍改变时，初始化本机所有玩家的OverheadWidget
		else
		{
			TArray<AActor*> AllPlayers;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), TAG_CHARACTER_BASE, AllPlayers);

			for (AActor* Player : AllPlayers)
			{
				if (ABaseCharacter* IgnoreBaseCharacter = Cast<ABaseCharacter>(Player))
				{
					if (UWidgetComponent* OverheadWidget = IgnoreBaseCharacter->OverheadWidget)
					{
						if (UOverheadWidget* OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject()))
						{
							OverheadWidgetClass->InitOverheadWidget();
						}
					}
				}
			}
		}
		
		return;
	}

	GetWorldTimerManager().SetTimerForNextTick([this]() {
		InitOverheadWidget();
	});
}

void ABasePlayerState::ServerSetHumanCharacterName_Implementation(EHumanCharacterName Name)
{
	HumanCharacterName = Name;
}

void ABasePlayerState::ServerSetMutantCharacterName_Implementation(EMutantCharacterName Name)
{
	MutantCharacterName = Name;
}

void ABasePlayerState::SetMutantCharacterName(EMutantCharacterName Name)
{
	MutantCharacterName = Name;
}

void ABasePlayerState::ServerSetAccountId_Implementation(FUniqueNetIdRepl TempAccountIdRepl)
{
	AccountIdRepl = TempAccountIdRepl;
}

void ABasePlayerState::AddDamage(float TempDamage)
{
	Damage += TempDamage;
}

void ABasePlayerState::OnRep_Damage()
{
}

void ABasePlayerState::AddDefeat()
{
	Defeat++;
}

void ABasePlayerState::OnRep_Defeat()
{
}

void ABasePlayerState::AddKillStreak()
{
	KillStreak++;
	OnKillStreakChange();

	GetWorldTimerManager().SetTimer(ResetKillStreakTimerHandle, this, &ThisClass::ResetKillStreak, 7.f);
}

void ABasePlayerState::OnRep_KillStreak()
{
	OnKillStreakChange();
}

void ABasePlayerState::ResetKillStreak()
{
	KillStreak = 0;
	OnKillStreakChange();
}

void ABasePlayerState::OnKillStreakChange()
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		BaseController->OnKillStreakChange.Broadcast(KillStreak);
	}
}
