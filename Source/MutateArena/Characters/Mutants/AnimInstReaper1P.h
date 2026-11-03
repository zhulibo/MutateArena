#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstReaper1P.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstReaper1P : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstReaper1P();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
