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
	class UMaterialInterface* BodyMaterial;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* DoorLeftMaterial;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* DoorRightMaterial;
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
	EContainerColor CurrentColor;

	UPROPERTY(EditAnywhere)
	bool bShowFrontLeftDoor;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowFrontLeftDoor", UIMin = "-180", UIMax = "180"))
	float FrontLeftDoorAngle;

	UPROPERTY(EditAnywhere)
	bool bShowFrontRightDoor;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowFrontRightDoor", UIMin = "-180", UIMax = "180"))
	float FrontRightDoorAngle;

	UPROPERTY(EditAnywhere)
	bool bShowBackLeftDoor;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowBackLeftDoor", UIMin = "-180", UIMax = "180"))
	float BackLeftDoorAngle;

	UPROPERTY(EditAnywhere)
	bool bShowBackRightDoor;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bShowBackRightDoor", UIMin = "-180", UIMax = "180"))
	float BackRightDoorAngle;
	
};
