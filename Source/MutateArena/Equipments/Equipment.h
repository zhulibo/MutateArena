#pragma once

#include "CoreMinimal.h"
#include "Data/EquipmentType.h"
#include "GameFramework/Actor.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "Equipment.generated.h"

class UAnimInstEquipment;
class UEquipmentSkinAsset;
class UEquipmentAsset;
struct FGameplayAbilitySpecHandle;
class UAbilitySystemComponent;
class UGameplayAbility;
enum class ETeam : uint8;

USTRUCT()
struct FEquipmentDropInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	UPROPERTY()
	FVector LinearVelocity = FVector::ZeroVector;

	// 确保同一把枪多次掉落时，每次都能触发 OnRep
	UPROPERTY()
	uint16 Sequence = 0;
};

UCLASS()
class MUTATEARENA_API AEquipment : public AActor
{
	GENERATED_BODY()

public:
	AEquipment();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_Owner() override;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* CollisionSphere;
	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* EquipmentMesh;
	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* EquipmentMesh1P;
	UPROPERTY(VisibleAnywhere)
	USphereComponent* OverlapSphere;
	
	// 值由 AHumanCharacter::ServerSpawnEquipments_Implementation 创建时传入
	UPROPERTY(Replicated)
	EEquipmentName EquipmentName = EEquipmentName::None;

	UPROPERTY(Transient)
	TObjectPtr<const UEquipmentAsset> EquipmentAsset = nullptr;

	UAnimInstEquipment* GetEquipmentAnimInst();
	UAnimInstEquipment* GetEquipmentAnimInst1P();

	void SetHiddenMesh(bool bInHidden);

	// 统一刷新 1P/3P 武器网格体的显隐与阴影状态（供切枪、拾取、丢弃及观战视角切换调用）
	void UpdateViewMode();

protected:
	bool bIsHidden = false;

public:
	virtual void OnStartSwapOut()
	{
	};

protected:
	UPROPERTY()
	UAnimInstEquipment* AnimInstEquipment;
	UPROPERTY()
	UAnimInstEquipment* AnimInstEquipment1P;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;

	UFUNCTION()
	virtual void OnAreaSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	UPROPERTY(ReplicatedUsing = OnRep_EquipmentState)
	EEquipmentState EquipmentState = EEquipmentState::Dropped;
	UFUNCTION()
	virtual void OnRep_EquipmentState(EEquipmentState OldState);

	virtual void OnGiven(class AHumanCharacter* HumanChar);
	void HandleDrop();
	virtual void Drop();
	UPROPERTY(ReplicatedUsing = OnRep_DropInfo)
	FEquipmentDropInfo DropInfo;
	UFUNCTION()
	void OnRep_DropInfo();
	
protected:
	UPROPERTY(Replicated)
	ETeam OwnerTeam = ETeam::NoTeam;
	void SetOwnerTeam();
	
public:
	const UEquipmentAsset* EnsureEquipmentAsset();

protected:
	FTimerHandle AreaSphereTimerHandle;
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