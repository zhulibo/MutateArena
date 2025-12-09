#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "LoginController.generated.h"

UCLASS()
class MUTATEARENA_API ALoginController : public AModularPlayerController
{
	GENERATED_BODY()

public:

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ULoginLayout> LoginLayoutClass;
	void AddLoginLayout();

};
