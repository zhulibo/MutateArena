#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "Online/CoreOnline.h"
#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "TeamType.h"
#include "BasePlayerState.generated.h"

enum class ETeam : uint8;
enum class EHumanCharacterName: uint8;
enum class EMutantCharacterName: uint8;
enum class ECommonInputType : uint8;

UCLASS()
class MUTATEARENA_API ABasePlayerState : public AModularPlayerState, public IAbilitySystemInterface
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
	float GetMaxWalkSpeed();
	float GetJumpZVelocity();

	UPROPERTY(Replicated)
	ECommonInputType InputType;
	
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
	
	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::NoTeam;
	virtual void SetTeam(ETeam TempTeam);
protected:
	UFUNCTION()
	virtual void OnRep_Team();
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Damage)
	float Damage = 0.f;
	virtual void AddDamage(float TempDamage);
protected:
	UFUNCTION()
	virtual void OnRep_Damage();
	UFUNCTION(Client, Reliable)
	void ClientOnAddDamage(float TempDamage); // 使用RPC快速显示UI
	virtual void ClientOnAddDamage_Implementation(float TempDamage);
	void ShowDamageUI(float TempDamage);
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Death)
	int32 Death = 0;
	void AddDeath();
protected:
	UFUNCTION()
	void OnRep_Death();
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Survive)
	int32 Survive = 0;
	virtual void AddSurvive(int32 TempSurvive);
protected:
	UFUNCTION()
	virtual void OnRep_Survive();
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Infect)
	int32 Infect = 0;
	virtual void AddInfect(int32 TempInfect);
protected:
	UFUNCTION()
	virtual void OnRep_Infect();

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
