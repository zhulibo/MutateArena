#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstCaller.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstCaller : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstCaller();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};