#pragma once

#include "CoreMinimal.h"
#include "MutateArena/Characters/AnimInstMutant.h"
#include "AnimInstCorpseDriver1P.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstCorpseDriver1P : public UAnimInstMutant
{
	GENERATED_BODY()

public:
	UAnimInstCorpseDriver1P();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
};