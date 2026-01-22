#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NotifyLineButton.generated.h"

UCLASS()
class MUTATEARENA_API UNotifyLineButton : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Notify;

};
