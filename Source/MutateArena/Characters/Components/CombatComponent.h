#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

enum class ECombatState : uint8;
enum class EEquipmentType : uint8;

// TODO Refactor the hard-coded state machine into a GAS-based architecture.
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
	AHumanCharacter* HumanCharacter;
	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class UAnimInstance_Human* HumanAnimInstance;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

public:
	UPROPERTY()
	ECombatState CombatState;
	
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

protected:
	FVector HitTarget;
	void TraceUnderCrosshair(FHitResult& TraceHitResult);

public:
	UPROPERTY()
	class AWeapon* PrimaryEquipment;
	UPROPERTY()
	AWeapon* SecondaryEquipment;
	UPROPERTY()
	class AMelee* MeleeEquipment;
	UPROPERTY()
	class AThrowing* ThrowingEquipment;
	UPROPERTY()
	EEquipmentType CurEquipmentType;
	UPROPERTY()
	EEquipmentType LastEquipmentType;
	class AEquipment* GetCurEquipment();
	AEquipment* GetLastEquipment();
	// 如果正在使用主、副武器，返回正在使用的武器，否则返空。
	AWeapon* GetCurWeapon();
protected:
	AEquipment* GetEquipmentByType(EEquipmentType EquipmentType);
	bool HasEquippedThisTypeEquipment(EEquipmentType EquipmentType);

	// 装备
	void EquipEquipment(AEquipment* Equipment);
	UFUNCTION(Server, Reliable)
	void ServerEquipEquipment(AEquipment* Equipment);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipEquipment(AEquipment* Equipment);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipEquipment2(AEquipment* Equipment);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReplaceCurEquipment(AEquipment* Equipment);
	void AssignEquipment(AEquipment* Equipment);
	void LocalEquipEquipment(AEquipment* Equipment);
	void AttachToBodySocket(AEquipment* Equipment);

	// 切换
	void SwapEquipment(EEquipmentType EquipmentType);
	UFUNCTION(Server, Reliable)
	void ServerSwapEquipment(EEquipmentType EquipmentType);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSwapEquipment(EEquipmentType EquipmentType);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSwapEquipment2(EEquipmentType EquipmentType);
	void LocalSwapEquipment(EEquipmentType EquipmentType);
	void PreLocalSwapEquipment();
	bool bIsSwappingOut = false;
	void PlaySwapOutMontage(AEquipment* NewEquipment);
public:
	UPROPERTY()
	AEquipment* TargetEquipment;
	void PlaySwapInMontage(bool bInterrupted, AEquipment* NewEquipment);
protected:
	void PlaySwapOutMontage_Reverse(bool bInterrupted, AEquipment* NewEquipment);
public:
	void FinishSwap();
	
protected:
	// 使用
	void UseEquipment(AEquipment* Equipment);
public:
	void AttachToRightHand(AEquipment* Equipment);
	void AttachToLeftHand(AEquipment* Equipment);
	void AttachToHand(AEquipment* Equipment, FString SocketNameSuffix);

	// 瞄准
	UPROPERTY()
	bool bIsAiming = false;
protected:
	void SetAiming(bool TempBIsAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool TempBIsAiming);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetAiming(bool TempBIsAiming);
	void LocalSetAiming(bool TempBIsAiming);
	void CalcCameraToScopeRotator();

	// 射击
	bool bCanFire = true;
public:
	int32 CurShotCount = 1;
protected:
	bool bFireButtonPressed;
	void StartFire();
	void StopFire();
	void Fire();
	bool CanFire();
	FTimerHandle LoadNewBulletTimerHandle;
	void LoadNewBulletFinished();
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw);
	void LocalFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw);
	void PlayFireMontage();

	// 装弹
	void Reload();
	UFUNCTION(Server, Reliable)
	void ServerReload();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReload();
	void LocalReload();
	void PlayReloadMontage();
	void JumpToShotgunEnd();
public:
	void ShellReload();
	void FinishReload();
	
protected:
	// 丢弃
	void DropEquipment(EEquipmentType EquipmentType);
	UFUNCTION(Server, Reliable)
	void ServerDropEquipment(EEquipmentType EquipmentType);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDropEquipment(EEquipmentType EquipmentType);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDropEquipment2(EEquipmentType EquipmentType);
	void LocalDropEquipment(EEquipmentType EquipmentType);
	
public:
	// 销毁
	UFUNCTION(Server, Reliable)
	void ServerDestroyEquipments();
protected:
	UFUNCTION()
	void LocalDestroyEquipments();

	// 近战攻击
	void MeleeAttack(ECombatState TempCombatState);
	UFUNCTION(Server, Reliable)
	void ServerMeleeAttack(ECombatState TempCombatState);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastMeleeAttack(ECombatState TempCombatState);
	void LocalMeleeAttack(ECombatState TempCombatState);
	bool bIsCombo = false;
	UFUNCTION(Server, Reliable)
	void ServerSetIsCombo(bool TempBIsCombo);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetIsCombo(bool TempBIsCombo);
public:
	void AttackFirstSectionEnd();
	void SetAttackCollisionEnabled(bool bIsEnabled);

protected:
	// 投掷
	void Throw();
	UFUNCTION(Server, Reliable)
	void ServerThrow();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastThrow();
	void LocalThrow();
public:
	UFUNCTION(Server, Reliable)
	void ServerThrowOut();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastThrowOut();
	void ThrowOut();

};
