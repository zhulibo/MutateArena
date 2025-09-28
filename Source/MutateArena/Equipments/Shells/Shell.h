#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shell.generated.h"

UCLASS()
class MUTATEARENA_API AShell : public AActor
{
	GENERATED_BODY()

public:
	AShell();

	UPROPERTY()
	FVector InitVelocity;

protected:
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
