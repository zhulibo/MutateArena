#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Equipment.h"
#include "Melee.generated.h"

class UAnimMontage;
class UAssetSubsystem; // 前向声明

UCLASS()
class MUTATEARENA_API AMelee : public AEquipment
{
	GENERATED_BODY()

public:
	AMelee();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	UAnimMontage* LightAttackMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* LightAttackMontage_E;

	UPROPERTY(EditAnywhere)
	UAnimMontage* HeavyAttackMontage_C;
	UPROPERTY(EditAnywhere)
	UAnimMontage* HeavyAttackMontage_E;

	virtual void OnStartSwapOut() override;
	void SetAttackCollisionEnabled(bool bIsEnabled);

	void ClearHitEnemies();

	UPROPERTY(Replicated)
	bool bIsLightAttack = false;

protected:
	UPROPERTY()
	float LightAttackDamage;
	UPROPERTY()
	float HeavyAttackDamage;

public:
	virtual void OnEquip(class AHumanCharacter* HumanChar) override;

protected:
	// 设置扫掠检测的目标类型
	void SetMeleeTraceObjectTypes();

	UPROPERTY()
	TArray<AActor*> HitEnemies;

	UFUNCTION(Server, Reliable)
	void ServerApplyDamage(AActor* OtherActor, AHumanCharacter* InstigatorCharacter, float Damage);

	UFUNCTION()
	virtual void DropBlood(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, float Damage,
	                       const FHitResult& TraceHitResult);

	// 播放击中墙体/静态物体的贴花与音效 (新增 TraceDirection 参数)
	void SpawnHitWallEffects(const FHitResult& HitResult, const FVector& TraceDirection);

	// 扫掠检测插槽名称数组
	UPROPERTY()
	TArray<FName> TraceSockets;

	// 扫掠检测的球体半径
	UPROPERTY()
	float TraceRadius;

	// 是否正在进行攻击判定
	UPROPERTY()
	bool bIsAttacking = false;

	// 记录上一帧插槽位置，用于帧间连线扫掠
	TMap<FName, FVector> PreviousSocketLocations;

	// 扫掠检测对象类型参数
	FCollisionObjectQueryParams MeleeObjectQueryParams;

	// 处理检测击中 (新增 TraceDirection 参数)
	void ProcessMeleeHit(const FHitResult& HitResult, const FVector& TraceDirection);
};
