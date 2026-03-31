#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutationContainer.generated.h"

UCLASS()
class MUTATEARENA_API UMutationContainer : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	void OnTeamChange(ETeam Team);
	void OnHUDStateChange(EHUDState HUDState);

	UPROPERTY(meta = (BindWidget))
	class UCommonHUD* CommonHUD;

	UPROPERTY(meta = (BindWidget))
	class UMutationHUD* MutationHUD;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UMutationHuman> MutationHuman;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UMutationMutant> MutationMutant;

	UPROPERTY(meta = (BindWidget))
	class UScoreboard* Scoreboard;

	UPROPERTY(meta = (BindWidget))
	class URadialMenuContainer* RadialMenuContainer;
	
};
