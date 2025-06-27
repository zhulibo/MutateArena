#pragma once

#include "CoreMinimal.h"
#include "BasePlayerState.h"
#include "MutationPlayerState.generated.h"

UCLASS()
class MUTATEARENA_API AMutationPlayerState : public ABasePlayerState
{
	GENERATED_BODY()

public:
	AMutationPlayerState();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Reset() override;

	UPROPERTY()
	class AMutationController* MutationController;

public:
	UPROPERTY(Replicated)
	bool bKilledByMelee = false;
	
	virtual void SetTeam(ETeam TempTeam) override;
protected:
	virtual void OnRep_Team() override;

public:
	virtual void AddDamage(float TempDamage) override;
protected:
	virtual void OnRep_Damage() override;
	float BaseDamage = 0;
	void Show1000DamageUI();
	
public:
	UPROPERTY(ReplicatedUsing = OnRep_Rage)
	float Rage = 0.f;
	void SetRage(float TempRage);
protected:
	UFUNCTION()
	void OnRep_Rage();
	void ApplyLevelUpEffect();

};
