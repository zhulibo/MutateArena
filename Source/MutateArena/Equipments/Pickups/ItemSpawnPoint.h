#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawnPoint.generated.h"

// 定义生成物类型的枚举
UENUM(BlueprintType)
enum class ESpawnItemType : uint8
{
	Pickup,
	HerbRage,
	HerbRepelReceived,
};

UCLASS()
class MUTATEARENA_API AItemSpawnPoint : public AActor
{
	GENERATED_BODY()
    
public:    
	AItemSpawnPoint();

	virtual void OnConstruction(const FTransform& Transform) override;
	
	// 在关卡编辑器中通过下拉菜单选择生成类型
	UPROPERTY(EditAnywhere)
	ESpawnItemType ItemType;

protected:
	UPROPERTY(VisibleAnywhere)
	class UArrowComponent* ArrowComponent;
};