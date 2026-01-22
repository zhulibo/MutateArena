#include "NotifyLineButton.h"

void UNotifyLineButton::NativeConstruct()
{
	Super::NativeConstruct();

	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]() {
		this->RemoveFromParent();
	});
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 10.f, false);
}
