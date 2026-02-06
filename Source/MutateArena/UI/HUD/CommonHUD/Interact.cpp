#include "Interact.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Components/ProgressBar.h"
#include "MutateArena/System/UISubsystem.h"

void UInteract::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ProgressBar->SetFillColorAndOpacity(C_GREEN);

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnInteractStarted.AddUObject(this, &ThisClass::OnInteractStarted);
		UISubsystem->OnInteractEnded.AddUObject(this, &ThisClass::OnInteractEnded);
	}
}

void UInteract::NativeConstruct()
{
	Super::NativeConstruct();
}

void UInteract::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ProgressBar->GetPercent() < 3)
	{
		ProgressBar->SetPercent(ProgressBar->GetPercent() + InDeltaTime / 3.f);
	}
}

void UInteract::OnInteractStarted()
{
	ProgressBar->SetPercent(0.f);
	ProgressBar->SetVisibility(ESlateVisibility::Visible);
}

void UInteract::OnInteractEnded()
{
	ProgressBar->SetVisibility(ESlateVisibility::Hidden);
	ProgressBar->SetPercent(0.f);
}

