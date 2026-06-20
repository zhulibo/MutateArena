#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Container.generated.h"

UENUM(BlueprintType)
enum class EContainerColor : uint8
{
	Blue	UMETA(DisplayName = "Blue"),
	Green	UMETA(DisplayName = "Green"),
	Red		UMETA(DisplayName = "Red"),
	White	UMETA(DisplayName = "White"),
	Yellow	UMETA(DisplayName = "Yellow")
};

USTRUCT(BlueprintType)
struct FContainerSkin
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* BodyMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* DoorLeftMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* DoorRightMaterial = nullptr;
};

UCLASS()
class MUTATEARENA_API AContainer : public AActor
{
	GENERATED_BODY()
	
public:	
	AContainer();
	
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* BodyMesh;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FrontLeftDoor;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FrontRightDoor;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BackLeftDoor;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BackRightDoor;

	UPROPERTY(EditDefaultsOnly)
	TMap<EContainerColor, FContainerSkin> SkinMap;

	UPROPERTY(EditAnywhere)
	EContainerColor CurrentColor = EContainerColor::White;

	UPROPERTY(EditAnywhere)
	bool bShowFrontLeftDoor = true;
	UPROPERTY(EditAnywhere)
	bool bShowFrontRightDoor = true;
	UPROPERTY(EditAnywhere)
	bool bShowBackLeftDoor = true;
	UPROPERTY(EditAnywhere)
	bool bShowBackRightDoor = true;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowFrontLeftDoor", UIMin = "-180", UIMax = "180"))
	float FrontLeftDoorAngle = 0.f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowFrontRightDoor", UIMin = "-180", UIMax = "180"))
	float FrontRightDoorAngle = 0.f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowBackLeftDoor", UIMin = "-180", UIMax = "180"))
	float BackLeftDoorAngle = 0.f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowBackRightDoor", UIMin = "-180", UIMax = "180"))
	float BackRightDoorAngle = 0.f;
	
};
