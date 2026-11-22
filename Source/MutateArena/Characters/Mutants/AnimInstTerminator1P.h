#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstTerminator1P.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstTerminator1P : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstTerminator1P();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
