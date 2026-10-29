#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstSmoker.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstSmoker : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstSmoker();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
};
