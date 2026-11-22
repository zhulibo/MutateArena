#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstTerminator.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstTerminator : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstTerminator();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
