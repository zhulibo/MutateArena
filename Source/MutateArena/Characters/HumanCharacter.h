#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "MutantCharacter.h"
#include "Data/CharacterType.h"
#include "HumanCharacter.generated.h"

enum class EEquipmentName : uint8;
enum class EEquipmentType : uint8;

UCLASS()
class MUTATEARENA_API AHumanCharacter : public ABaseCharacter, public IInteractable
{
	GENERATED_BODY()

public:
	AHumanCharacter();

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* CombatComp;
	UPROPERTY(VisibleAnywhere)
	class URecoilComponent* RecoilComp;
	UPROPERTY(VisibleAnywhere)
	class UCrosshairComponent* CrosshairComp;
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

public:
	virtual void OnASCInit() override;
	void OnMaxWalkSpeedChanged(const FOnAttributeChangeData& Data);
	void OnSwappingTagChanged(FGameplayTag GameplayTag, int NewCount);
	void OnAimingTagChanged(FGameplayTag GameplayTag, int NewCount);
	void UpdateMaxWalkSpeed();
	
protected:
	void AimButtonPressed(const FInputActionValue& Value);
	void AimButtonReleased(const FInputActionValue& Value);
public:
	void FireButtonPressed(const FInputActionValue& Value);
	void FireButtonReleased(const FInputActionValue& Value);
	void ReloadButtonPressed(const FInputActionValue& Value);
protected:
	void DropButtonPressed(const FInputActionValue& Value);
	void SwapLastEquipmentButtonPressed(const FInputActionValue& Value);
	void SwapBetweenPrimarySecondaryEquipmentButtonPressed(const FInputActionValue& Value);
public:
	void SendSwapEquipmentEvent(EEquipmentType TargetEquipmentType);
	
	UFUNCTION(Client, Reliable)
	void ClientSwapEquipmentWhenPickupFailed(EEquipmentType FallbackEquipmentType);
	void EquipOverlappingEquipment_Server(class AEquipment* Equipment);
	UFUNCTION(Server, Reliable)
	void ServerGivePickupEquipment(class APickupEquipment* PickupEquipment);

	UPROPERTY()
	bool bCanSwitchLoadout = true;
	void TrySwitchLoadout();

protected:
	virtual void OnLocallyControllerReady() override;

	void ApplyLoadout();
	UFUNCTION(Server, Reliable)
	void ServerSpawnEquipments(EEquipmentName Primary, EEquipmentName Secondary, EEquipmentName Melee, EEquipmentName Throwing);
	EEquipmentName GetEquipmentName(int32 LoadoutIndex, EEquipmentType EquipmentType);

	UFUNCTION()
	void HumanReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser);
public:
	virtual void OnRep_bIsDead() override;
	void MutationDead(bool bNeedSpawn, ESpawnMutantReason SpawnMutantReason = ESpawnMutantReason::Fall);
	void MeleeDead();
	void TeamDeadMatchDead();
protected:
	void HandleDead();

public:
	UPROPERTY(ReplicatedUsing = OnRep_bIsImmune)
	bool bIsImmune = false;
	UFUNCTION()
	void BecomeImmune();
	UFUNCTION()
	void OnRep_bIsImmune();
	
};
