#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "MutantCharacter.h"
#include "Data/CharacterType.h"
#include "Interfaces/InteractableTarget.h"
#include "HumanCharacter.generated.h"

enum class EEquipmentName : uint8;
enum class EEquipmentType : uint8;
enum class ECombatState : uint8;

UCLASS()
class MUTATEARENA_API AHumanCharacter : public ABaseCharacter, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AHumanCharacter();

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* CombatComponent;
	UPROPERTY(VisibleAnywhere)
	class URecoilComponent* RecoilComponent;
	UPROPERTY(VisibleAnywhere)
	class UCrosshairComponent* CrosshairComponent;
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY()
	class AMutationMode* MutationMode;
	UPROPERTY()
	class AMeleeMode* MeleeMode;
	UPROPERTY()
	class ATeamDeadMatchMode* TeamDeadMatchMode;
	UPROPERTY()
	class AMeleeGameState* MeleeGameState;

public:
	virtual void OnASCInit() override;
	void OnMaxWalkSpeedChanged(const FOnAttributeChangeData& Data);
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
public:
	void SwapPrimaryEquipmentButtonPressed();
	void SwapSecondaryEquipmentButtonPressed();
	void SwapMeleeEquipmentButtonPressed();
	void SwapThrowingEquipmentButtonPressed();
protected:
	void SwapLastEquipmentButtonPressed(const FInputActionValue& Value);
	void SwapBetweenPrimarySecondaryEquipmentButtonPressed(const FInputActionValue& Value);

public:
	void OnServerDropEquipment();
	void EquipOverlappingEquipment(class AEquipment* Equipment);
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

	UPROPERTY(ReplicatedUsing = OnRep_DefaultPrimary)
	class AWeapon* DefaultPrimary;
	UPROPERTY(ReplicatedUsing = OnRep_DefaultSecondary)
	AWeapon* DefaultSecondary;
	UPROPERTY(ReplicatedUsing = OnRep_DefaultMelee)
	class AMelee* DefaultMelee;
	UPROPERTY(ReplicatedUsing = OnRep_DefaultThrowing)
	class AThrowing* DefaultThrowing;
	UFUNCTION()
	void OnRep_DefaultPrimary();
	UFUNCTION()
	void OnRep_DefaultSecondary();
	UFUNCTION()
	void OnRep_DefaultMelee();
	UFUNCTION()
	void OnRep_DefaultThrowing();

	UFUNCTION()
	void HumanReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* AttackerController, AActor* DamageCauser);
public:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastMutationDead(bool bNeedSpawn, ESpawnMutantReason SpawnMutantReason = ESpawnMutantReason::Fall);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastMeleeDead();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastTeamDeadMatchDead();
protected:
	void HandleDead();

public:
	UPROPERTY(ReplicatedUsing = OnRep_bIsImmune)
	bool bIsImmune = false;
	UFUNCTION()
	virtual void OnInteractMutantSuccess(class AMutantCharacter* MutantCharacter) override;
protected:
	UFUNCTION(Server, Reliable)
	void ServerOnImmune(AMutantCharacter* MutantCharacter);
	UFUNCTION()
	void OnRep_bIsImmune();
	
public:
	UCombatComponent* GetCombatComponent() const { return CombatComponent; }
	
};
