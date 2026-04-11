#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "MutateArena/Interfaces/Interactable.h"
#include "MutantCharacter.generated.h"

enum class EMutantCharacterName : uint8;
enum class ESpawnMutantReason : uint8;

UCLASS()
class MUTATEARENA_API AMutantCharacter : public ABaseCharacter, public IInteractable
{
	GENERATED_BODY()

public:
	AMutantCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* RightHandCapsule;
	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* LeftHandCapsule;

	UPROPERTY(EditAnywhere)
	UAnimMontage* LightAttackMontage;
	UPROPERTY(EditAnywhere)
	UAnimMontage* HeavyAttackMontage;

	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* MutantBornSound;
	
	float LastRepelTime = 0.f;
	UPROPERTY()
	float RepelCooldown = 0.2f;
	void ApplySuppressionForce(FVector HitDirection, float PushStrength);
	UFUNCTION(Client, Reliable)
	void Client_PredictSuppressionForce(FVector HitDirection, float PushStrength);
	void ExecuteRootMotionPush(FVector HitDirection, float PushStrength, float Duration);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Destroyed() override;

protected:
	UPROPERTY()
	class AMutationMode* MutationMode;
	UPROPERTY()
	class AMutationController* MutationController;
	UPROPERTY()
	class UAnimInstMutant* AnimInstMutant;
	
	virtual void OnLocallyControllerReady() override;
	
public:
	// GAS
	UPROPERTY()
	ESpawnMutantReason SpawnMutantReason;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameplayAbilityBase> SkillAbility;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> LevelUpEffect;
protected:
	virtual void OnASCInit() override;
	void OnLocalSkillCooldownTagChanged(FGameplayTag GameplayTag, int32 TagCount);
	void OnLocalCharacterLevelChanged(const FOnAttributeChangeData& Data);
	void OnMaxWalkSpeedChanged(const FOnAttributeChangeData& Data);
	virtual void OnHealthChanged(const FOnAttributeChangeData& Data) override;
	void SkillButtonPressed(const FInputActionValue& Value);
	virtual void MoveStarted(const FInputActionValue& Value) override;
	virtual void MoveCompleted(const FInputActionValue& Value) override;
	UPROPERTY()
	FTimerHandle StillTimerHandle;
	bool bHasActivateRestoreAbility = false;
	void ActivateRestoreAbility();
	void EndRestoreAbility();

	// 攻击
public:
	virtual void LightAttackButtonPressed(const FInputActionValue& Value);
	virtual void LightAttackButtonReleased(const FInputActionValue& Value);
	virtual void HeavyAttackButtonPressed(const FInputActionValue& Value);
	virtual void HeavyAttackButtonReleased(const FInputActionValue& Value);
	UPROPERTY(Replicated)
	bool bIsLightAttack = false;
protected:
	UPROPERTY()
	float LightAttackDamage = 0.f;
	UPROPERTY()
	float HeavyAttackDamage = 0.f;
public:
	UPROPERTY()
	TArray<AActor*> RightHandHitEnemies;
	UPROPERTY()
	TArray<AActor*> LeftHandHitEnemies;
	virtual void RightHandAttackBegin();
	virtual void RightHandAttackEnd();
	virtual void LeftHandAttackBegin();
	virtual void LeftHandAttackEnd();
protected:
	UFUNCTION()
	virtual void OnRightHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	virtual void OnLeftHandCapsuleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
public:
	UFUNCTION(Server, Reliable)
	void ServerApplyDamage(AActor* OtherActor, float Damage);
	virtual void ServerApplyDamage_Implementation(AActor* OtherActor, float Damage);
protected:
	UFUNCTION()
	virtual void DropBlood(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, float Damage);

	UFUNCTION()
	void MutantReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser);
public:
	virtual void OnRep_bIsDead() override;
	UPROPERTY(Replicated)
	bool bKilledByMelee = false;
	void MutantDead(bool bTempKilledByMelee);
protected:
	void RemoveMappingContext();

public:
	UPROPERTY()
	EMutantCharacterName MutantCharacterName;
	UFUNCTION(Server, Reliable)
	void ServerSelectMutant(EMutantCharacterName TempMutantCharacterName);

	// 交互
	virtual bool CanInteract(ABaseCharacter* Interactor) override;
	virtual void OnInteract_Server(ABaseCharacter* Interactor) override;
protected:
	void AddRageOnSuck();
	UPROPERTY(ReplicatedUsing = OnRep_bSuckedDry)
	bool bSuckedDry = false;
	void SetIsSuckedDry(bool TempBSuckedDry);
	UFUNCTION()
	void OnRep_bSuckedDry();
	void SetDeadMaterial();
	
};
