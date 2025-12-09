#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CommonActivatableWidget.h"
#include "LayoutBase.generated.h"

UCLASS()
class MUTATEARENA_API ULayoutBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UPROPERTY()
	TMap<FGameplayTag, class UCommonActivatableWidgetStack*> LayersMap;
	void RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetStack* Stack);
	
public:
	UFUNCTION()
	UCommonActivatableWidgetStack* GetStackByTag(FGameplayTag LayerTag) const;
	
};
