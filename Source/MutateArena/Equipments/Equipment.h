#pragma once

#include "CoreMinimal.h"
#include "Data/EquipmentType.h"
#include "GameFramework/Actor.h"
#include "Equipment.generated.h"

class UAnimInstEquipment;
class UEquipmentSkinAsset;
class UEquipmentAsset;
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	TObjectPtr<const UEquipmentAsset> EquipmentAsset;

	UAnimInstEquipment* GetEquipmentAnimInst();
	UAnimInstEquipment* GetEquipmentAnimInst1P();

	void SetHiddenMesh(bool TempbIsHidden);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_IsHidden)
	bool bIsHidden = false;
	UFUNCTION()
	void OnRep_IsHidden();

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
	EEquipmentState EquipmentState;
	UFUNCTION()
	virtual void OnRep_EquipmentState(EEquipmentState OldState);

	virtual void OnGiven(class AHumanCharacter* HumanChar);
	void HandleDrop();
	virtual void Drop();

protected:
	UPROPERTY(replicated)
	ETeam OwnerTeam;
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
