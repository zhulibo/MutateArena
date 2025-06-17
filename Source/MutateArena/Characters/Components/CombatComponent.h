#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

enum class ECombatState : uint8;
enum class EEquipmentType : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MUTATEARENA_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	friend class AHumanCharacter;

	class AEquipment* GetCurEquipment();
	AEquipment* GetLastEquipment();
	class AWeapon* GetUsingWeapon(); // 如果正在使用主、副武器，返回正在使用的武器，否则返空。

	void FinishSwap();

	void AttachToRightHand(AEquipment* Equipment);
	void AttachToLeftHand(AEquipment* Equipment);
	void AttachToHand(AEquipment* Equipment, FString SocketNameSuffix);

	void ShellReload();
	void FinishReload();

	void AttackFirstSectionEnd();
	void SetAttackCollisionEnabled(bool bIsEnabled);

	UFUNCTION(Server, Reliable)
	void ServerThrowOut();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastThrowOut();
	void ThrowOut();

protected:
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

	UPROPERTY()
	ECombatState CombatState;

	UPROPERTY()
	float BaseWalkSpeed = 600.f;

	FVector HitTarget;
	void TraceUnderCrosshair(FHitResult& TraceHitResult);

	float DefaultFOV;
	float CurrentFOV;
	void InterpFOV(float DeltaSeconds);

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
	void PlaySwapOutMontage(AEquipment* NewEquipment);
	void PlaySwapInMontage(bool bInterrupted, AEquipment* NewEquipment);

	// 使用
	void UseEquipment(AEquipment* Equipment);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReplaceCurEquipment(AEquipment* Equipment);

	// 瞄准
	UPROPERTY()
	bool bIsAiming = false;
	void SetAiming(bool TempBIsAiming);
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool TempBIsAiming);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetAiming(bool TempBIsAiming);
	void LocalSetAiming(bool TempBIsAiming);

	// 射击
	bool bCanFire = true;
	bool bIsFirstShot = true;
	bool bFireButtonPressed;
	void StartFire();
	void StopFire();
	void Fire();
	bool CanFire();
	FTimerHandle LoadNewBulletTimerHandle;
	void LoadNewBulletFinished();
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor);
	void LocalFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor);
	void PlayFireMontage();

	float RecoilVertical = 0.f;
	float RecoilHorizontal = 0.f;

	float RecoilIncCostTime = 0.f;
	float RecoilDecCostTime = 0.f;

	float RecoilVerticalLastTick = 0.f;
	float RecoilHorizontalLastTick = 0.f;
	float RecoilVerticalLastTick2 = 0.f;
	float RecoilHorizontalLastTick2 = 0.f;

	// 装弹
	void Reload();
	UFUNCTION(Server, Reliable)
	void ServerReload();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReload();
	void LocalReload();
	void PlayReloadMontage();
	void JumpToShotgunEnd();

	// 丢弃
	void DropEquipment(EEquipmentType EquipmentType);
	UFUNCTION(Server, Reliable)
	void ServerDropEquipment(EEquipmentType EquipmentType);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDropEquipment(EEquipmentType EquipmentType);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDropEquipment2(EEquipmentType EquipmentType);
	void LocalDropEquipment(EEquipmentType EquipmentType);

	// 销毁
	UFUNCTION(Server, Reliable)
	void ServerDestroyEquipments();
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
	
	// 投掷
	void Throw();
	UFUNCTION(Server, Reliable)
	void ServerThrow();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastThrow();
	void LocalThrow();

public:
	FORCEINLINE AWeapon* GetPrimaryEquipment() const { return PrimaryEquipment; }
	FORCEINLINE AWeapon* GetSecondaryEquipment() const { return SecondaryEquipment; }
	FORCEINLINE AMelee* GetMeleeEquipment() const { return MeleeEquipment; }
	FORCEINLINE AThrowing* GetThrowingEquipment() const { return ThrowingEquipment; }
	FORCEINLINE EEquipmentType GetCurEquipmentType() const { return CurEquipmentType; }
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	FORCEINLINE bool IsAiming() const { return bIsAiming; }
	FORCEINLINE bool IsFireButtonPressed() const { return bFireButtonPressed; }
	FORCEINLINE bool IsFirstShot() const { return bIsFirstShot; }

};
