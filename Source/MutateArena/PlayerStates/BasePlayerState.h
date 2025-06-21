#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "Online/CoreOnline.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "BasePlayerState.generated.h"

enum class ETeam : uint8;
enum class EHumanCharacterName: uint8;
enum class EMutantCharacterName: uint8;

UCLASS()
class MUTATEARENA_API ABasePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABasePlayerState();
	
	UPROPERTY()
	class UMAAbilitySystemComponent* AbilitySystemComponent;
	UPROPERTY()
	class UAttributeSetBase* AttributeSetBase;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSetBase* GetAttributeSetBase();
	float GetMaxHealth();
	float GetHealth();
	float GetDamageReceivedMul();
	float GetRepelReceivedMul();
	float GetCharacterLevel();
	float GetJumpZVelocity();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Reset() override;
	virtual void Destroyed() override;

	virtual void InitData();

	UPROPERTY()
	class ABaseCharacter* BaseCharacter;
	UPROPERTY()
	class ABaseController* BaseController;
	
	UFUNCTION(Server, Reliable)
	void ServerSetHumanCharacterName(EHumanCharacterName Name);
public:
	UPROPERTY(Replicated)
	EHumanCharacterName HumanCharacterName;
	UPROPERTY(Replicated)
	EMutantCharacterName MutantCharacterName;
	UFUNCTION(Server, Reliable)
	void ServerSetMutantCharacterName(EMutantCharacterName Name);
	UFUNCTION()
	void SetMutantCharacterName(EMutantCharacterName Name);
	
protected:
	UPROPERTY(Replicated)
	FUniqueNetIdRepl AccountIdRepl;
	UFUNCTION(Server, Reliable)
	void ServerSetAccountId(FUniqueNetIdRepl TempAccountIdRepl);
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team;
	virtual void SetTeam(ETeam TempTeam);
	void InitOverheadWidget();
protected:
	UFUNCTION()
	virtual void OnRep_Team();
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Damage)
	float Damage;
	virtual void AddDamage(float TempDamage);
protected:
	UFUNCTION()
	virtual void OnRep_Damage();

public:
	UPROPERTY(ReplicatedUsing = OnRep_Defeat)
	int32 Defeat;
	void AddDefeat();
protected:
	UFUNCTION()
	void OnRep_Defeat();

	UPROPERTY(ReplicatedUsing = OnRep_KillStreak)
	int32 KillStreak = 0;
public:
	void ResetKillStreak();
	void AddKillStreak();
	void OnKillStreakChange();
protected:
	UFUNCTION()
	void OnRep_KillStreak();
	FTimerHandle ResetKillStreakTimerHandle;

public:
	FORCEINLINE EHumanCharacterName GetHumanCharacterName() const { return HumanCharacterName; }
	FORCEINLINE EMutantCharacterName GetMutantCharacterName() const { return MutantCharacterName; }

};
