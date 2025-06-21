#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Interfaces/InteractableTarget.h"
#include "MutantCharacter.generated.h"

UENUM(BlueprintType)
enum class EMutantState : uint8
{
	Ready,
	LightAttacking,
	HeavyAttacking,
};

enum class EMutantCharacterName : uint8;
enum class ESpawnMutantReason : uint8;

UCLASS()
class MUTATEARENA_API AMutantCharacter : public ABaseCharacter, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AMutantCharacter();

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

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRepel(FVector ImpulseVector);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Destroyed() override;

	UPROPERTY()
	class AMutationMode* MutationMode;
	EMutantState MutantState;
	UPROPERTY()
	class AMutationController* MutationController;
	UPROPERTY()
	class UAnimInstance_Mutant* AnimInstance_Mutant;

public:
	// GAS
	UPROPERTY()
	ESpawnMutantReason SpawnMutantReason;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameplayAbilityBase> SkillAbility;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> SkillEffect;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> LevelUpEffect;
protected:
	virtual void OnAbilitySystemComponentInit() override;
	void OnLocalSkillCooldownTagChanged(FGameplayTag GameplayTag, int32 TagCount);
	void OnLocalCharacterLevelChanged(const FOnAttributeChangeData& Data);
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
	virtual void LightAttackButtonPressed(const FInputActionValue& Value);
	virtual void LightAttackButtonReleased(const FInputActionValue& Value);
	virtual void HeavyAttackButtonPressed(const FInputActionValue& Value);
	virtual void HeavyAttackButtonReleased(const FInputActionValue& Value);
	UFUNCTION(Server, Reliable)
	void ServerLightAttack();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLightAttack();
	void LocalLightAttack();
	UFUNCTION(Server, Reliable)
	void ServerHeavyAttack();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHeavyAttack();
	void LocalHeavyAttack();
	
	bool bIsCombo = false;
	UFUNCTION(Server, Reliable)
	void ServerSetIsCombo(bool TempBIsCombo);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetIsCombo(bool TempBIsCombo);
public:
	void AttackFirstSectionEnd();
protected:
	UPROPERTY()
	float LightAttackDamage = 0.f;
	UPROPERTY()
	float HeavyAttackDamage = 0.f;
	UPROPERTY()
	TArray<AActor*> RightHandHitEnemies;
	UPROPERTY()
	TArray<AActor*> LeftHandHitEnemies;
public:
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
protected:
	UFUNCTION()
	virtual void DropBlood(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, float Damage);

	UFUNCTION()
	void MutantReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser);
public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDead(bool bKilledByMelee);
protected:
	void MutantRespawn(bool bKilledByMelee);
	void RemoveMappingContext();

public:
	UPROPERTY()
	EMutantCharacterName MutantCharacterName;
	UFUNCTION(Server, Reliable)
	void ServerSelectMutant(EMutantCharacterName TempMutantCharacterName);

protected:
	// 交互
	UPROPERTY(ReplicatedUsing = OnRep_bSuckedDry)
	bool bSuckedDry = false;
	UFUNCTION()
	void OnRep_bSuckedDry();
	void SetDeadMaterial();
	virtual bool CanInteract() override;
	virtual void OnInteract(ABaseCharacter* BaseCharacter) override;
	virtual void OnInteractOnServer() override;
	UFUNCTION()
	void SetIsSuckedDry(bool TempBSuckedDry);
	UFUNCTION()
	virtual void OnInteractMutantSuccess(AMutantCharacter* MutantCharacter) override;
	UFUNCTION(Server, Reliable)
	void ServerOnSuck(AMutantCharacter* MutantCharacter);

};
