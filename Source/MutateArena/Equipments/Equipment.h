#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Equipment.generated.h"

enum class ETeam : uint8;
enum class EEquipmentName : uint8;
enum class EEquipmentCate : uint8;
enum class EEquipmentType : uint8;
enum class EEquipmentState : uint8;

UCLASS()
class MUTATEARENA_API AEquipment : public AActor
{
	GENERATED_BODY()

public:
	AEquipment();

	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapInMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapInMontage_E;

	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapOutMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* SwapOutMontage_E;

	class UAnimInstance_Equipment* GetEquipmentAnimInstance();

	virtual void OnEquip();
	virtual void OnDrop();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHiddenMesh();

	virtual void OnStartSwapOut() {};

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* CollisionSphere;
	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* EquipmentMesh;
	UPROPERTY(VisibleAnywhere)
	USphereComponent* OverlapSphere;

	UPROPERTY()
	UAnimInstance_Equipment* EquipmentAnimInstance;
	
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

	UFUNCTION()
	virtual void OnAreaSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 蓝图和DataTable都需要设置，用于相互对应。
	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentName;
	UPROPERTY()
	EEquipmentName EquipmentParentName;
	UPROPERTY()
	EEquipmentCate EquipmentCate;
	UPROPERTY()
	EEquipmentType EquipmentType;

	UPROPERTY()
	EEquipmentState EquipmentState;
	UPROPERTY()
	float MoveSpeedMul = 1.f;

	UPROPERTY()
	class AHumanCharacter* HumanCharacter;
	UPROPERTY()
	class ABaseController* BaseController;

	UPROPERTY(replicated)
	ETeam OwnerTeam;
	void SetOwnerTeam();

	void SetAreaSphereCollision();
	FTimerHandle DestroyEquipmentTimerHandle;
	void DestroyEquipment();

public:
	FORCEINLINE USkeletalMeshComponent* GetEquipmentMesh() const { return EquipmentMesh; }

	FORCEINLINE EEquipmentName GetEquipmentName() const { return EquipmentName; }
	FORCEINLINE EEquipmentName GetEquipmentParentName() const { return EquipmentParentName; }
	FORCEINLINE EEquipmentCate GetEquipmentCate() const { return EquipmentCate; }
	FORCEINLINE EEquipmentType GetEquipmentType() const { return EquipmentType; }

	FORCEINLINE EEquipmentState GetEquipmentState() const { return EquipmentState; }
	FORCEINLINE float GetMoveSpeedMul() const { return MoveSpeedMul; }

};
