#pragma once

#include "CoreMinimal.h"
#include "RadialMenuBase.h"
#include "RadialMenuEquipment.generated.h"

enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API URadialMenuEquipment : public URadialMenuBase
{
	GENERATED_BODY()

public:
	void RefreshRadialMenu();
	
protected:
	virtual void NativeOnInitialized() override;
	
	void OnTeamChange(ETeam Team);
	void SetHumanRadialMenuText();
	void SetMutantRadialMenuText();

};
