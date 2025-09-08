#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "LoginController.generated.h"

UCLASS()
class MUTATEARENA_API ALoginController : public AModularPlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class ULoginLayout* LoginLayout;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ULoginLayout> LoginLayoutClass;
	void AddLoginLayout();

};
