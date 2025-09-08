#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "MenuController.generated.h"

UCLASS()
class MUTATEARENA_API AMenuController : public AModularPlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UMenuLayout* MenuLayout;
	// ServerContainer位于Switcher中不便访问，故缓存ServerContainer中的ServerStack于此。
	UPROPERTY()
	class UCommonActivatableWidgetStack* ServerStack;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UMenuLayout> MenuLayoutClass;
	void AddMenuLayout();

};
