#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstCorpseDriver.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstCorpseDriver : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstCorpseDriver();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
};