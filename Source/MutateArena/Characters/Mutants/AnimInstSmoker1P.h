#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstSmoker1P.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstSmoker1P : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstSmoker1P();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
