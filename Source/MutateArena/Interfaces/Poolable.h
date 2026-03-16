#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Poolable.generated.h"

UINTERFACE(MinimalAPI)
class UPoolable : public UInterface
{
	GENERATED_BODY()
};

class MUTATEARENA_API IPoolable
{
	GENERATED_BODY()

public:
	virtual void OnSpawnedFromPool() = 0;
	virtual void OnReturnedToPool() = 0;
	
};
