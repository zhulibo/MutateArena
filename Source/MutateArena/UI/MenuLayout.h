#pragma once

#include "CoreMinimal.h"
#include "LayoutBase.h"
#include "MenuLayout.generated.h"

UCLASS()
class MUTATEARENA_API UMenuLayout : public ULayoutBase
{
	GENERATED_BODY()

public:
	// 菜单层
	UPROPERTY(meta = (BindWidget))
	class UCommonActivatableWidgetStack* MenuStack;
	// 模态层
	UPROPERTY(meta = (BindWidget))
	UCommonActivatableWidgetStack* ModalStack;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMenu> MenuClass;

};
