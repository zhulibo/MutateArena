#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MutateArena/System/Interfaces/Poolable.h"
#include "Shell.generated.h"

UCLASS()
class MUTATEARENA_API AShell : public AActor, public IPoolable
{
	GENERATED_BODY()

public:
	AShell();

	UPROPERTY()
	FVector InitVelocity;
	
	virtual void OnSpawnedFromPool() override;
	virtual void OnReturnedToPool() override;
	void LaunchShell(const FVector& CharacterVelocity);
protected:
	FTimerHandle LifeSpanTimer;
	FTimerHandle CollisionDelayTimer;
	UFUNCTION()
	void ReturnToPool();
	
	virtual void BeginPlay() override;
	
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ShellMesh;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* ShellSound_Concrete;
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* ShellSound_Dirt;
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* ShellSound_Metal;
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* ShellSound_Wood;
	
};
