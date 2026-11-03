#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "Online/CoreOnline.h"
#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "TeamType.h"
#include "BasePlayerState.generated.h"

enum class ETeam : uint8;
enum class EHumanName: uint8;
enum class EMutantName: uint8;
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
	float GetBodyResistance();

	UPROPERTY(Replicated)
	ECommonInputType InputType;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	EHumanName HumanName;
protected:
	UFUNCTION(Server, Reliable)
	void ServerSetHumanName(EHumanName Name);
public:
	UPROPERTY(Replicated)
	EMutantName MutantName;
	UFUNCTION(Server, Reliable)
	void ServerSetMutantName(EMutantName Name);
	UFUNCTION()
	void SetMutantName(EMutantName Name);

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
	virtual void OnRep_Damage(float OldValue);
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
	UPROPERTY(Replicated)
	EDNA2 DNA1 = EDNA2::None;
	UPROPERTY(Replicated)
	EDNA2 DNA2 = EDNA2::None;
	UFUNCTION(Server, Reliable)
	void ServerSetDNA(EDNA2 TempDNA1, EDNA2 TempDNA2);
	void ApplyDNAGameplayEffect(EDNA2 DNA);

};
