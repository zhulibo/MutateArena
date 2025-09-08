#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DataSubsystem.generated.h"

UCLASS()
class MUTATEARENA_API UDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

};
