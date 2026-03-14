#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "CombatComponent.generated.h"

enum class EEquipmentType : uint8;

UCLASS()
class MUTATEARENA_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	friend class AHumanCharacter;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	AHumanCharacter* HumanChar;
	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class UAnimInstHuman* HumanAnimInst;
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
	// 如果正在使用主、副武器，返回正在使用的武器，否则返空。
	class AWeapon* GetCurWeapon();
	// 如果正在使用近战武器，返回正在使用的近战武器，否则返空。
	class AMelee* GetCurMelee();
	AEquipment* GetEquipmentByType(EEquipmentType EquipmentType);
protected:
	bool HasEquippedEquipment(EEquipmentType EquipmentType);
public:
	UPROPERTY(ReplicatedUsing = OnRep_PrimaryEquipment)
	class AWeapon* PrimaryEquipment;
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryEquipment)
	AWeapon* SecondaryEquipment;
	UPROPERTY(ReplicatedUsing = OnRep_MeleeEquipment)
	class AMelee* MeleeEquipment;
	UPROPERTY(ReplicatedUsing = OnRep_ThrowingEquipment)
	class AThrowing* ThrowingEquipment;
	UPROPERTY(ReplicatedUsing = OnRep_CurEquipmentType)
	EEquipmentType CurEquipmentType;
	UPROPERTY(Replicated)
	EEquipmentType LastEquipmentType;
protected:
	UFUNCTION()
	void OnRep_PrimaryEquipment();
	UFUNCTION()
	void OnRep_SecondaryEquipment();
	UFUNCTION()
	void OnRep_MeleeEquipment();
	UFUNCTION()
	void OnRep_ThrowingEquipment();
	UFUNCTION()
	void OnRep_CurEquipmentType(EEquipmentType OldEquipmentType);

	// 装备
public:
	void EquipEquipment(AEquipment* NewEquipment, bool bIsCurrent = false);
protected:
	void AssignEquipment(AEquipment* Equipment);
public:
	UFUNCTION(Client, Unreliable)
	void ClientPlayEquipSound();

	// 切换
	UPROPERTY()
	EEquipmentType DesiredEquipmentType = EEquipmentType::None;
	void InstantSwap(EEquipmentType TargetType);

	void AttachToBody(AEquipment* Equipment);
	void AttachToHand(AEquipment* Equipment, FName SocketNameSuffix);

	// 瞄准
	bool IsAiming();
	void LocalSetAiming(bool TempBIsAiming);
protected:
	void CalcCameraToScopeRotator();

	// 射击
public:
	void TraceUnderCrosshair(FHitResult& TraceHitResult);
	int32 CurShotCount = 1;

	// 销毁
	UFUNCTION(Server, Reliable)
	void ServerDestroyEquipments();
protected:
	UFUNCTION()
	void LocalDestroyEquipments(bool bIsDestroyPrimary = true);

	// 近战
public:
	void SetAttackCollisionEnabled(bool bIsEnabled);
	
};
