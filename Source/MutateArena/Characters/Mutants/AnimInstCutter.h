#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstCutter.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstCutter : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstCutter();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
};
