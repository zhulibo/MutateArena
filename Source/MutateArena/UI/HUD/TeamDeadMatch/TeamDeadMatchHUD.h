#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "TeamDeadMatchHUD.generated.h"

UCLASS()
class MUTATEARENA_API UTeamDeadMatchHUD : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* MatchCountdown;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Team1Score;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Team2Score;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Health;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Ammo;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CarriedAmmo;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	
	void OnMatchCountdownChange(int32 CountdownTime);
	void OnHumanHealthChange(float TempHealth);
	void OnAmmoChange(int32 TempAmmo);
	void OnCarriedAmmoChange(int32 TempCarriedAmmo);
	void OnTeamScoreChange(float Score, ETeam Team);

};
