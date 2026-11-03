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

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
