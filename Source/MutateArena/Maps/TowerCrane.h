#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TowerCrane.generated.h"

UCLASS()
class MUTATEARENA_API ATowerCrane : public AActor
{
	GENERATED_BODY()
    
public:    
	ATowerCrane();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:    
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere)
	class USceneComponent* RootComp;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* TopMesh;

	UPROPERTY(EditAnywhere)
	float RotationInterval = 20.0f;

	UPROPERTY(EditAnywhere)
	float RotationAngle = -40.0f;

	UPROPERTY(EditAnywhere)
	float RotationInterpSpeed = 5.0f;

	// 使用布尔值在 初始位置 和 目标角度 之间切换
	UPROPERTY(ReplicatedUsing = OnRep_bIsRotated)
	bool bIsRotated = false;

	UFUNCTION()
	void OnRep_bIsRotated();

	void ToggleRotation();

private:
	FTimerHandle RotationTimerHandle;
	FRotator BaseRotation;
	FRotator TargetRotation;

	void UpdateTargetRotation();
};