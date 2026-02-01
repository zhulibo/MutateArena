#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LoadoutContent.generated.h"

UCLASS()
class MUTATEARENA_API ULoadoutContent : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class ULoadoutItem* Primary;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Secondary;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Melee;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Throwing;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Tool;
	UPROPERTY(meta = (BindWidget))
	ULoadoutItem* Tonic;

protected:
	virtual void NativeOnInitialized() override;
	
};
