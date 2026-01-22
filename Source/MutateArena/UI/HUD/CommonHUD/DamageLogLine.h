#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "DamageLogLine.generated.h"

UCLASS()
class MUTATEARENA_API UDamageLogLine : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Damage;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* AppearanceAnim;
	
};
