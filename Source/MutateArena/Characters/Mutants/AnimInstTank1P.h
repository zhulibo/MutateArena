#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstTank1P.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstTank1P : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstTank1P();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
