#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "MACheatManager.generated.h"

UCLASS()
class MUTATEARENA_API UMACheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void GiveEquipment(FString EquipmentName);
	
	UFUNCTION(Exec)
	void SwitchTeam(int32 TeamNum = -1);
	
};
