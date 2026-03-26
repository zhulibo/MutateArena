#include "BasePlayerState.h"

#include "MetaSoundSource.h"
#include "Net/UnrealNetwork.h"
#include "TeamType.h"
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
#include "MutateArena/System/Tags/ProjectTags.h"

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
}

void ABasePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InputType);
	DOREPLIFETIME(ThisClass, HumanCharacterName);
	DOREPLIFETIME(ThisClass, MutantCharacterName);
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

	if (!BaseController) BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		UStorageSubsystem* StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		if (StorageSubsystem && StorageSubsystem->CacheLoadout)
		{
			ServerSetHumanCharacterName(StorageSubsystem->CacheLoadout->HumanCharacterName);
			ServerSetMutantCharacterName(StorageSubsystem->CacheLoadout->MutantCharacterName);
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
	return AttributeSetBase ? AttributeSetBase->GetCharacterLevel() : 0.f;
}

float ABasePlayerState::GetMaxWalkSpeed()
{
	return AttributeSetBase ? AttributeSetBase->GetMaxWalkSpeed() : 0.f;
}

float ABasePlayerState::GetJumpZVelocity()
{
	return AttributeSetBase ? AttributeSetBase->GetJumpZVelocity() : 0.f;
}

void ABasePlayerState::SetTeam(ETeam TempTeam)
{
	Team = TempTeam;

	OnRep_Team();
}

void ABasePlayerState::OnRep_Team()
{
	BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BaseCharacter)
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
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwner());
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
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwner());
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
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwner());
	if (BaseController && BaseController->IsLocalController())
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			UISubsystem->OnKillStreakChange.Broadcast(KillStreak);
		}
	}
}

void ABasePlayerState::ServerSetDNA_Implementation(EDNA TempDNA1, EDNA TempDNA2)
{
	DNA1 = TempDNA1;
	DNA2 = TempDNA2;
	
	ApplyDNAGameplayEffect(TempDNA1);
	ApplyDNAGameplayEffect(TempDNA2);
}

void ABasePlayerState::ApplyDNAGameplayEffect(EDNA DNA)
{
	if (!AbilitySystemComponent) return;
	
	UStorageSubsystem* StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (!StorageSubsystem || !StorageSubsystem->CacheLoadout) return;
	
	UDNAAsset2* DNAAsset = StorageSubsystem->GetDNAAssetByType(DNA);
	if (!DNAAsset) return;

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddInstigator(this, this);
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DNAAsset->GEClass, 1.f, Context);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
