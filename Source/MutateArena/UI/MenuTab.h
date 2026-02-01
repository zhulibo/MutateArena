#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "MenuTab.generated.h"

UCLASS()
class MUTATEARENA_API UMenuTab : public UCommonTabListWidgetBase
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
	class UCommonActivatableWidgetSwitcher* MenuSwitcher;
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* TabButtonContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UCommonButton> TabButtonClass;
	UPROPERTY(EditInstanceOnly)
	TArray<class UCommonActivatableWidget*> TabContents;
	void LinkSwitcher();
	
	bool bIsInitialized = true;
	UFUNCTION()
	void HandleOnTabSelected(FName TabId);

};
