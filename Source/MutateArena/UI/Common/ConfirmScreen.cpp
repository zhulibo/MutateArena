#include "ConfirmScreen.h"

#include "CommonButton.h"
#include "CommonTextBlock.h"

void UConfirmScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

UWidget* UConfirmScreen::NativeGetDesiredFocusTarget() const
{
	return ConfirmButton;
}

void UConfirmScreen::Setup(FText TempMsg, FConfirmScreenComplete TempCB)
{
	Msg->SetText(TempMsg);
	CB = TempCB;

	ConfirmButton->OnClicked().Clear();
	CancelButton->OnClicked().Clear();
	
	ConfirmButton->OnClicked().AddUObject(this, &ThisClass::Confirm);
	CancelButton->OnClicked().AddUObject(this, &ThisClass::Cancel);
}

void UConfirmScreen::Confirm()
{
	CB.ExecuteIfBound(EMsgResult::Confirm);
}

void UConfirmScreen::Cancel()
{
	DeactivateWidget();

	CB.ExecuteIfBound(EMsgResult::Cancel);
}
