#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "MAGameplayCueManager.generated.h"

UCLASS()
class MUTATEARENA_API UMAGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()

public:
	static UMAGameplayCueManager* Get();

protected:
	// virtual bool ShouldAsyncLoadRuntimeObjectLibraries() const override;

};
