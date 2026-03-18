#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "RadialMenuContainer.generated.h"

enum class ETeam : uint8;

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
	class URadialMenuEquipment* RadialMenuEquipment;
	UPROPERTY(meta = (BindWidget))
	class URadialMenuRadio* RadialMenuRadio;
	UPROPERTY(meta = (BindWidget))
	class URadialMenuPaint* RadialMenuPaint;

	UPROPERTY()
	class AHumanCharacter* HumanCharacter;
	UPROPERTY()
	class AMutantCharacter* MutantCharacter;

	struct FMenuCategory
	{
		UCommonTextBlock* TitleText;
		class URadialMenuBase* RadialMenu;
	};

	TArray<FMenuCategory> MenuCategories;
	int32 CurrentCategoryIndex = 0;

	void ShowRadialMenu(bool bIsShow);

	void ShowRadialMenuInternal();
	void SwitchRadialMenu();
	void SelectRadialMenu(double X, double Y);
	void CloseRadialMenuInternal();

	class URadialMenuBase* GetActiveRadialMenu();
	
};
