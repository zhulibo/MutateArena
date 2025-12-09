#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "MenuController.generated.h"

UCLASS()
class MUTATEARENA_API AMenuController : public AModularPlayerController
{
	GENERATED_BODY()

public:

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMenuLayout> MenuLayoutClass;
	void AddMenuLayout();

};
