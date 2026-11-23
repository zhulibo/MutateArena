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
	float DefaultFOV = 90.f;
	// 瞄准进度 (0 = 腰射, 1 = 完全瞄准 | 同时驱动镜头 FOV 缩放与后坐力子弹偏移衰减)
	float AimingProgress = 0.f;
protected:
	float CachedAimFOVMul = 1.f;
	float CachedADSTime = 0.2f;
	bool bPrevEffectiveAiming = false;
	void UpdateLocalAiming(float DeltaSeconds);
public:

	class AEquipment* GetCurEquipment();
	AEquipment* GetLastEquipment();
	// 如果正在使用主副武器，返回主副武器，否则返空。
	class AWeapon* GetCurWeapon();
	// 如果正在使用近战武器，返回近战武器，否则返空。
	class AMelee* GetCurMelee();
	AEquipment* GetEquipmentBySlot(EEquipmentSlot EquipmentSlot);
protected:
	bool HasEquipmentInSlot(EEquipmentSlot EquipmentSlot);
public:
	UPROPERTY(ReplicatedUsing = OnRep_Slot1Equipment)
	class AWeapon* Slot1Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_Slot2Equipment)
	AWeapon* Slot2Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_Slot3Equipment)
	class AMelee* Slot3Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_Slot4Equipment)
	class AThrowable* Slot4Equipment;
	UPROPERTY(ReplicatedUsing = OnRep_CurEquipmentSlot)
	EEquipmentSlot CurEquipmentSlot = EEquipmentSlot::None;
	UPROPERTY(Replicated)
	EEquipmentSlot LastEquipmentSlot = EEquipmentSlot::None;
protected:
	UFUNCTION()
	void OnRep_Slot1Equipment(AWeapon* OldEquipment);
	UFUNCTION()
	void OnRep_Slot2Equipment(AWeapon* OldEquipment);
	UFUNCTION()
	void OnRep_Slot3Equipment(AMelee* OldEquipment);
	UFUNCTION()
	void OnRep_Slot4Equipment(AThrowable* OldEquipment);
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

	// 瞄准（服务端通过 COND_SkipOwner 仅同步给远端模拟代理，主控端与服务端由 GA_Equipment_Aim 本地直接维护以防止延迟回弹）
	UPROPERTY(Replicated)
	bool bIsAiming = false;

	// 射击
	int32 CurShotCount = 1;
	// 每次本地开火生成不同种子，再通过开火数据同步到服务器和观察端（FGameplayCueParameters::AbilityLevel 网络序列化上限为 31）。
	int32 LastSpreadSeed = 0;
	int32 GenerateSpreadSeed()
	{
		LastSpreadSeed = (LastSpreadSeed % 31) + 1;
		return LastSpreadSeed;
	}

	// 销毁
	UFUNCTION(Server, Reliable)
	void ServerDestroyEquipments(bool bIsDestroySlot1 = true);

	// 近战
	void SetAttackCollisionEnabled(bool bIsEnabled);
	
};
