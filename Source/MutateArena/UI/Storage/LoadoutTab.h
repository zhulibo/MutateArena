#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "LoadoutTab.generated.h"

UCLASS()
class MUTATEARENA_API ULoadoutTab : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	class UCommonActionWidget* LeftTabAction;
	UPROPERTY(meta = (BindWidget))
	UCommonActionWidget* RightTabAction;

	UPROPERTY(EditInstanceOnly)
	class UCommonActivatableWidgetSwitcher* LoadoutSwitcher;
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* TabButtonContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UCommonButton> TabButtonClass;
	UPROPERTY(EditInstanceOnly)
	TArray<class ULoadoutContent*> TabContents;
	void LinkSwitcher();

};
