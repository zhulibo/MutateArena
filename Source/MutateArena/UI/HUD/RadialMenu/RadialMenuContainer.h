#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "RadialMenuContainer.generated.h"

UCLASS()
class MUTATEARENA_API URadialMenuContainer : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* TitleEquipment;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* TitleRadio;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* TitlePaint;

	UPROPERTY(meta = (BindWidget))
	class URadialMenuBase* RadialMenuEquipment;
	UPROPERTY(meta = (BindWidget))
	URadialMenuBase* RadialMenuRadio;
	UPROPERTY(meta = (BindWidget))
	URadialMenuBase* RadialMenuPaint;

	void ShowRadialMenu(bool bIsShow);

	void ShowRadialMenuInternal();
	void ChangeRadialMenu();
	void SelectRadialMenu(double X, double Y);
	void CloseRadialMenuInternal();

	void OnTeamChange(ETeam TempTeam);
	void SetHumanRadialMenuText();
	void SetMutantRadialMenuText();

	int32 ActiveRadialMenuIndex = 1;
	URadialMenuBase* GetActiveRadialMenu();

	void SetSelectedItem();

	UPROPERTY()
	class AHumanCharacter* HumanCharacter;
	UPROPERTY()
	class AMutantCharacter* MutantCharacter;

};
