#include "BasePlayerState.h"

#include "MetaSoundSource.h"
#include "Net/UnrealNetwork.h"
#include "TeamType.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Abilities/MAAbilitySystemComponent.h"
#include "MutateArena/Abilities/AttributeSetBase.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Storage/DefaultConfig.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/System/GameplayTags.h"

ABasePlayerState::ABasePlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UMAAbilitySystemComponent>(TEXT("BaseAbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSetBase = CreateDefaultSubobject<UAttributeSetBase>(TEXT("BaseAttributeSet"));

	if (const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>())
	{
		HumanName = DefaultConfig->HumanName;
		MutantName = DefaultConfig->MutantName;
	}
}

void ABasePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InputType);
	DOREPLIFETIME(ThisClass, HumanName);
	DOREPLIFETIME(ThisClass, MutantName);
	DOREPLIFETIME(ThisClass, Team);
	DOREPLIFETIME(ThisClass, Damage);
	DOREPLIFETIME(ThisClass, Death);
	DOREPLIFETIME(ThisClass, Survive);
	DOREPLIFETIME(ThisClass, Infect);
	DOREPLIFETIME(ThisClass, KillStreak);
	DOREPLIFETIME(ThisClass, DNA1);
	DOREPLIFETIME(ThisClass, DNA2);
}

void ABasePlayerState::BeginPlay()
{
	Super::BeginPlay();

	ABaseController* BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		UStorageSubsystem* StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheLoadout)
		{
			ServerSetHumanName(StorageSubsystem->CacheLoadout->HumanName);
			ServerSetMutantName(StorageSubsystem->CacheLoadout->MutantName);
		}
	}
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
	return AttributeSetBase ? AttributeSetBase->GetCharacterLevel() : 1.f;
}

float ABasePlayerState::GetMaxWalkSpeed()
{
	return AttributeSetBase ? AttributeSetBase->GetMaxWalkSpeed() : 0.f;
}

float ABasePlayerState::GetJumpZVelocity()
{
	return AttributeSetBase ? AttributeSetBase->GetJumpZVelocity() : 0.f;
}

float ABasePlayerState::GetBodyResistance()
{
	return AttributeSetBase ? AttributeSetBase->GetBodyResistance() : 0.f;
}

void ABasePlayerState::SetTeam(ETeam TempTeam)
{
	Team = TempTeam;

	OnRep_Team();
}

void ABasePlayerState::OnRep_Team()
{
	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetPawn()))
	{
		BaseCharacter->bIsPlayerStateTeamReady = false;
	}

	if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(LocalPC->GetLocalPlayer()))
		{
			UISubsystem->OnOverheadWidgetNeedUpdate.Broadcast();
		}
	}

	ABaseController* BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			BaseController->InitHUD();

			UISubsystem->OnLocalTeamChange.Broadcast(Team);
		}
	}
}

void ABasePlayerState::ServerSetHumanName_Implementation(EHumanName Name)
{
	HumanName = Name;
}

void ABasePlayerState::ServerSetMutantName_Implementation(EMutantName Name)
{
	SetMutantName(Name);
}

void ABasePlayerState::SetMutantName(EMutantName Name)
{
	MutantName = Name;
}

void ABasePlayerState::AddDamage(float TempDamage)
{
	Damage += TempDamage;

	ShowDamageUI(TempDamage);

	ForceNetUpdate();
}

void ABasePlayerState::OnRep_Damage(float OldValue)
{
	ShowDamageUI(Damage - OldValue);
}

void ABasePlayerState::ShowDamageUI(float TempDamage)
{
	ABaseController* BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->OnCauseDamage.Broadcast(TempDamage);
		}
	}
}

void ABasePlayerState::AddDeath()
{
	Death++;
}

void ABasePlayerState::OnRep_Death()
{
}

void ABasePlayerState::AddSurvive(int32 TempSurvive)
{
	Survive += TempSurvive;
}

void ABasePlayerState::OnRep_Survive()
{
}

void ABasePlayerState::AddInfect(int32 TempInfect)
{
	Infect += TempInfect;
}

void ABasePlayerState::OnRep_Infect()
{
}

void ABasePlayerState::AddKillStreak()
{
	KillStreak++;

	OnRep_KillStreak();

	ForceNetUpdate(); // 快速播放音效

	GetWorldTimerManager().SetTimer(ResetKillStreakTimerHandle, this, &ThisClass::ResetKillStreak, 7.f);
}

void ABasePlayerState::OnRep_KillStreak()
{
	OnKillStreakChange();

	if (KillStreak > 0)
	{
		ABaseController* BaseController = Cast<ABaseController>(GetOwner());
		if (BaseController && BaseController->IsLocalController())
		{
			UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
			if (AssetSubsystem == nullptr || AssetSubsystem->CommonAsset == nullptr) return;

			if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(this, AssetSubsystem->CommonAsset->KillSound))
			{
				// AudioComponent->SetFloatParameter(TEXT("Index"), 1);
			}
		}
	}
}

void ABasePlayerState::ResetKillStreak()
{
	KillStreak = 0;
}

void ABasePlayerState::OnKillStreakChange()
{
	ABaseController* BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->OnKillStreakChange.Broadcast(KillStreak);
		}
	}
}

void ABasePlayerState::ServerSetDNA_Implementation(EDNA2 TempDNA1, EDNA2 TempDNA2)
{
	DNA1 = TempDNA1;
	DNA2 = TempDNA2;

	ApplyDNAGameplayEffect(TempDNA1);
	ApplyDNAGameplayEffect(TempDNA2);
}

void ABasePlayerState::ApplyDNAGameplayEffect(EDNA2 DNA)
{
	if (!AbilitySystemComponent) return;

	if (UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>())
	{
		if (UDNAAsset2* DNAAsset = AssetSubsystem->GetDNAAssetByType(DNA))
		{
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddInstigator(this, this);
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DNAAsset->GEClass, 1.f, Context);
			if (SpecHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}
