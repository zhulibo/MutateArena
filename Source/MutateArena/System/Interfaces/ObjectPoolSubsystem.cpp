#include "ObjectPoolSubsystem.h"
#include "Poolable.h"

AActor* UObjectPoolSubsystem::AcquireObject(TSubclassOf<AActor> ClassToSpawn, const FTransform& Transform)
{
	if (ClassToSpawn == nullptr) return nullptr;

	AActor* SpawnedActor = nullptr;
	FPoolArray* Pool = PoolMap.Find(ClassToSpawn);

	// 尝试从池中取出有效对象
	if (Pool && Pool->Objects.Num() > 0)
	{
		while (Pool->Objects.Num() > 0)
		{
			SpawnedActor = Pool->Objects.Pop();
			if (IsValid(SpawnedActor))
			{
				SpawnedActor->SetActorTransform(Transform);
				break;
			}
		}
	}

	// 如果池中没有可用对象，则生成新对象
	if (SpawnedActor == nullptr)
	{
		SpawnedActor = GetWorld()->SpawnActor<AActor>(ClassToSpawn, Transform);
	}

	// 唤醒对象
	if (IPoolable* PoolableActor = Cast<IPoolable>(SpawnedActor))
	{
		PoolableActor->OnSpawnedFromPool();
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

	// 加入对象池
	FPoolArray& Pool = PoolMap.FindOrAdd(ObjectToRelease->GetClass());
	Pool.Objects.AddUnique(ObjectToRelease);
}
