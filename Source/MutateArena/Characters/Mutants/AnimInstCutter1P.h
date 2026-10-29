#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstCutter1P.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstCutter1P : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstCutter1P();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
};
