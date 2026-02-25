#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolSubsystem.generated.h"

USTRUCT()
struct FPoolArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> Objects;
};

UCLASS()
class MUTATEARENA_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 从对象池获取对象（如果池空则生成新对象）
	UFUNCTION()
	AActor* AcquireObject(TSubclassOf<AActor> ClassToSpawn, const FTransform& Transform);

	// 将对象回收到对象池
	UFUNCTION()
	void ReleaseObject(AActor* ObjectToRelease);

private:
	UPROPERTY()
	TMap<UClass*, FPoolArray> PoolMap;
	
};
