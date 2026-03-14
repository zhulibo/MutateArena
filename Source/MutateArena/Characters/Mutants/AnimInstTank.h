#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstTank.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstTank : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstTank();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
};
