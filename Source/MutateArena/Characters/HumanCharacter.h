#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "MutantCharacter.h"
#include "Data/CharacterType.h"
#include "HumanCharacter.generated.h"

enum class EEquipmentName : uint8;
enum class EEquipmentSlot : uint8;

UCLASS()
class MUTATEARENA_API AHumanCharacter : public ABaseCharacter, public IInteractable
{
	GENERATED_BODY()

public:
	AHumanCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* CombatComp;
	UPROPERTY(VisibleAnywhere)
	class URecoilComponent* RecoilComp;
	UPROPERTY(VisibleAnywhere)
	class UCrosshairComponent* CrosshairComp;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;

protected:
	virtual void OnASCInit() override;
public:
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
	void SwapLastSlotEquipmentButtonPressed(const FInputActionValue& Value);
	void SwapBetweenSlot1Slot2EquipmentButtonPressed(const FInputActionValue& Value);
public:
	void SendSwapEquipmentEvent(EEquipmentSlot TargetEquipmentSlot);
	
	UFUNCTION(Client, Reliable)
	void ClientSwapEquipmentWhenPickupFailed(EEquipmentSlot FallbackEquipmentSlot);
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
	void ServerSpawnEquipments(EEquipmentName Slot1, EEquipmentName Slot2, EEquipmentName Slot3, EEquipmentName Slot4);

	UFUNCTION()
	void HumanReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser);
public:
	virtual void OnRep_bIsDead() override;
	void MutationDead(bool bNeedSpawn, ESpawnMutantReason SpawnMutantReason = ESpawnMutantReason::Fall);
	void MeleeDead();
	void TeamDeadMatchDead();
	virtual void OnDead() override;

	UPROPERTY(ReplicatedUsing = OnRep_bIsImmune)
	bool bIsImmune = false;
	UFUNCTION()
	void BecomeImmune();
	UFUNCTION()
	void OnRep_bIsImmune();
	
};
