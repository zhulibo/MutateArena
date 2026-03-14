#pragma once

#include "CoreMinimal.h"
#include "Data/EquipmentType.h"
#include "GameFramework/Actor.h"
#include "Equipment.generated.h"

struct FGameplayAbilitySpecHandle;
class UAbilitySystemComponent;
class UGameplayAbility;
enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API AEquipment : public AActor
{
	GENERATED_BODY()

public:
	AEquipment();
	
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* CollisionSphere;
	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* EquipmentMesh;
	UPROPERTY(VisibleAnywhere)
	USphereComponent* OverlapSphere;
	
	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapInMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapInMontage_E;

	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapOutMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapOutMontage_E;

	class UAnimInstEquipment* GetEquipmentAnimInst();

	void SetHiddenMesh(bool TempbIsHidden);
protected:
	UPROPERTY(ReplicatedUsing = OnRep_IsHidden)
	bool bIsHidden = false;
	UFUNCTION()
	void OnRep_IsHidden();

public:
	virtual void OnStartSwapOut() {};

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY()
	UAnimInstEquipment* EquipmentAnimInst;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;

	UFUNCTION()
	virtual void OnAreaSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentName = EEquipmentName::None; // 蓝图和DataTable都需要设置，用于相互对应。
	UPROPERTY()
	EEquipmentName EquipmentParentName = EEquipmentName::None;
	UPROPERTY()
	EEquipmentCate EquipmentCate = EEquipmentCate::None;
	UPROPERTY()
	EEquipmentType EquipmentType = EEquipmentType::None;

	UPROPERTY(ReplicatedUsing = OnRep_EquipmentState)
	EEquipmentState EquipmentState;
	UFUNCTION()
	virtual void OnRep_EquipmentState(EEquipmentState OldState);
	UPROPERTY()
	float WalkSpeedMul = 1.f;
	UPROPERTY()
	float AimingWalkSpeedMul = 1.f;

	virtual void OnEquip(class AHumanCharacter* HumanChar);
	void HandleDrop();
	virtual void Drop();
	
protected:
	UPROPERTY(replicated)
	ETeam OwnerTeam;
	void SetOwnerTeam();

protected:
	void SetAreaSphereCollision();
	
	FTimerHandle DestroyEquipmentTimerHandle;
	void DestroyEquipment();

public:
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayAbility>> Abilities;
	void GiveAbilities(UAbilitySystemComponent* ASC);
	void ClearAbilities(UAbilitySystemComponent* ASC);
protected:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	
};
