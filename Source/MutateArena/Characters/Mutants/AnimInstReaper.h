#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstReaper.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstReaper : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstReaper();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
