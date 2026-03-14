#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstGhost.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstGhost : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstGhost();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
};
