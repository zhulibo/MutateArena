#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolSubsystem.generated.h"

USTRUCT()
struct FPoolArray
{
	GENERATED_BODY()

	// 空闲对象栈（处于休眠状态）
	UPROPERTY()
	TArray<AActor*> InactiveObjects;

	// 活跃对象队列（正在场景中使用，索引 0 为最早借出的最老对象）
	UPROPERTY()
	TArray<AActor*> ActiveObjects;

	// 该类型在世界中允许同时存在的最大总数上限（0 表示不限制）
	int32 MaxPoolSize = 50;
};

UCLASS()
class MUTATEARENA_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void PrewarmPool(TSubclassOf<AActor> ClassToSpawn, int32 TargetPoolSize = 20, int32 MaxCap = 50);

	UFUNCTION()
	AActor* AcquireObject(TSubclassOf<AActor> ClassToSpawn, const FTransform& Transform);

	UFUNCTION()
	void ReleaseObject(AActor* ObjectToRelease);

private:
	UPROPERTY()
	TMap<UClass*, FPoolArray> PoolMap;
};
