#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "ConfirmScreen.generated.h"

UENUM(BlueprintType)
enum class EMsgResult : uint8
{
	Confirm,
	Cancel,
	Closed,
};

DECLARE_DELEGATE_OneParam(FConfirmScreenComplete, EMsgResult);

UCLASS()
class MUTATEARENA_API UConfirmScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void Setup(FText TempMsg, FConfirmScreenComplete TempCB);

	UPROPERTY(meta = (BindWidget))
	class UCommonButton* ConfirmButton;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* CancelButton;
	void Confirm();
	void Cancel();

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UCommonTextBlock> Msg;
	FConfirmScreenComplete CB;

};
