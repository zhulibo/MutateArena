#include "ObjectPoolSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MutateArena/Interfaces/Poolable.h"

void UObjectPoolSubsystem::PrewarmPool(TSubclassOf<AActor> ClassToSpawn, int32 TargetPoolSize, int32 MaxCap)
{
	if (GetWorld()->GetNetMode() == NM_DedicatedServer) return;
	if (ClassToSpawn == nullptr || TargetPoolSize <= 0) return;

	if (MaxCap > 0)
	{
		TargetPoolSize = FMath::Min(TargetPoolSize, MaxCap);
	}

	FPoolArray& Pool = PoolMap.FindOrAdd(ClassToSpawn);
	if (MaxCap > 0)
	{
		// 多把武器请求同一弹壳类时，保留更大的同屏上限配置
		Pool.MaxPoolSize = FMath::Max(Pool.MaxPoolSize, MaxCap);
	}

	// 用空闲数 + 活跃数判断，避免重复多生成
	int32 CurrentTotal = Pool.InactiveObjects.Num() + Pool.ActiveObjects.Num();
	int32 NeededCount = TargetPoolSize - CurrentTotal;
	if (NeededCount <= 0) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < NeededCount; ++i)
	{
		if (AActor* NewActor = GetWorld()->SpawnActor<AActor>(ClassToSpawn, FTransform::Identity, SpawnParams))
		{
			ReleaseObject(NewActor);
		}
	}
}

AActor* UObjectPoolSubsystem::AcquireObject(TSubclassOf<AActor> ClassToSpawn, const FTransform& Transform)
{
	if (ClassToSpawn == nullptr) return nullptr;

	FPoolArray& Pool = PoolMap.FindOrAdd(ClassToSpawn);
	AActor* SpawnedActor = nullptr;

	// 1. 优先尝试从空闲池中取出有效对象（显式传入 EAllowShrinking::No，避免频繁触发堆内存 Realloc）
	while (Pool.InactiveObjects.Num() > 0)
	{
		AActor* Candidate = Pool.InactiveObjects.Pop(EAllowShrinking::No);
		if (IsValid(Candidate))
		{
			SpawnedActor = Candidate;
			break;
		}
	}

	// 2. 如果空闲池为空，检查是否达到同屏硬上限
	if (SpawnedActor == nullptr)
	{
		// 清理可能被意外销毁的无效指针
		Pool.ActiveObjects.RemoveAll([](const AActor* Ptr) { return !IsValid(Ptr); });

		if (Pool.MaxPoolSize > 0 && Pool.ActiveObjects.Num() >= Pool.MaxPoolSize)
		{
			// 达到上限：强制抢占最早借出的老对象（索引 0）
			SpawnedActor = Pool.ActiveObjects[0];
			Pool.ActiveObjects.RemoveAt(0);

			// 先让老对象走一遍回收清理（清除残留速度和旧的回收 Timer）
			if (IPoolable* PoolableActor = Cast<IPoolable>(SpawnedActor))
			{
				PoolableActor->OnReturnedToPool();
			}
		}
		else
		{
			// 未达上限，正常生成新对象
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnedActor = GetWorld()->SpawnActor<AActor>(ClassToSpawn, Transform, SpawnParams);
		}
	}

	if (SpawnedActor)
	{
		// 使用 TeleportPhysics 瞬移到新位置，避免物理引擎产生错误动量
		SpawnedActor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);

		// 记录到活跃队列尾部
		Pool.ActiveObjects.Add(SpawnedActor);

		// 唤醒对象
		if (IPoolable* PoolableActor = Cast<IPoolable>(SpawnedActor))
		{
			PoolableActor->OnSpawnedFromPool();
		}
	}

	return SpawnedActor;
}

void UObjectPoolSubsystem::ReleaseObject(AActor* ObjectToRelease)
{
	if (!IsValid(ObjectToRelease)) return;

	// 让对象休眠
	if (IPoolable* PoolableActor = Cast<IPoolable>(ObjectToRelease))
	{
		PoolableActor->OnReturnedToPool();
	}

	// 从活跃队列移除，并加入空闲栈
	FPoolArray& Pool = PoolMap.FindOrAdd(ObjectToRelease->GetClass());
	Pool.ActiveObjects.RemoveSingle(ObjectToRelease);
	Pool.InactiveObjects.AddUnique(ObjectToRelease);
}
