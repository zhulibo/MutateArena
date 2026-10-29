#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "CombatComponent.generated.h"

enum class EEquipmentSlot : uint8;

UCLASS()
class MUTATEARENA_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	friend class AHumanCharacter;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY()
	AHumanCharacter* HumanChar;
	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class UAnimInstHuman* AnimInstHuman;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

public:
	float DefaultFOV;
	float TargetFOV;
protected:
	bool bNeedInterpFOV = false;
	float FOVInterpTime = 0.2f;
	float CurrentFOVInterpElapsed = 0.f;
	float StartFOV;
	void StartInterpFOV(float TempTargetFOV, float TempFOVInterpTime);
	void InterpFOV(float DeltaSeconds);
public:
	// 瞄准程度
	float AimingProgress = 0.f;

	class AEquipment* GetCurEquipment();
	AEquipment* GetLastEquipment();
	// 如果正在使用主副武器，返回主副武器，否则返空。
	class AWeapon* GetCurWeapon();
	// 如果正在使用近战武器，返回近战武器，否则返空。
	class AMelee* GetCurMelee();
	AEquipment* GetEquipmentByType(EEquipmentSlot EquipmentSlot);
protected:
	bool OwnEquipment(EEquipmentSlot EquipmentSlot);
public:
	UPROPERTY(ReplicatedUsing = OnRep_Slot1Equipment)
	class AWeapon* Slot1Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_Slot2Equipment)
	AWeapon* Slot2Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_Slot3Equipment)
	class AMelee* Slot3Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_Slot4Equipment)
	class AThrowing* Slot4Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_CurEquipmentSlot)
	EEquipmentSlot CurEquipmentSlot;
	UPROPERTY(Replicated)
	EEquipmentSlot LastEquipmentSlot;
protected:
	UFUNCTION()
	void OnRep_Slot1Equipment();
	UFUNCTION()
	void OnRep_Slot2Equipment();
	UFUNCTION()
	void OnRep_Slot3Equipment();
	UFUNCTION()
	void OnRep_Slot4Equipment();
	UFUNCTION()
	void OnRep_CurEquipmentSlot(EEquipmentSlot OldEquipmentSlot);

	// 给予
public:
	void GiveEquipment(AEquipment* NewEquipment);
	void HolsterEquipment(AEquipment* Equipment);
	void AttachToHand(AEquipment* Equipment, FName SocketNameSuffix);
	void RefreshCurEquipmentState();
	UFUNCTION(Client, Unreliable)
	void ClientPlayEquipSound();

	// 切换
	void InstantSwap(EEquipmentSlot TargetType);

	// 瞄准
	bool IsAiming();
	void LocalSetAiming(bool TempBIsAiming);

	// 射击
	void TraceUnderCrosshair(FHitResult& TraceHitResult);
	int32 CurShotCount = 1;

	// 销毁
	UFUNCTION(Server, Reliable)
	void ServerDestroyEquipments(bool bIsDestroySlot1 = true);

	// 近战
public:
	void SetAttackCollisionEnabled(bool bIsEnabled);
	
};