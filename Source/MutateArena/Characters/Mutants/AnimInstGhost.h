#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstGhost.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstGhost : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstGhost();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
