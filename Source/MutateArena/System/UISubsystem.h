#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UISubsystem.generated.h"

UCLASS()
class MUTATEARENA_API UUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
public:
	void RegisterGameLayout(class ULayoutBase* Layout);
	void UnregisterGameLayout(ULayoutBase* Layout);
	
	UFUNCTION()
	class UCommonActivatableWidgetStack* GetLayerStack(FGameplayTag LayerTag);
	
protected:
	TArray<TWeakObjectPtr<ULayoutBase>> ActiveLayouts;
	
};
