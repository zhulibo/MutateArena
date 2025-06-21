#include "Crosshair.h"

#include "CommonBorder.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Components/CanvasPanelSlot.h"

void UCrosshair::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Crosshair_L_Slot = Cast<UCanvasPanelSlot>(Crosshair_L->Slot);
	Crosshair_R_Slot = Cast<UCanvasPanelSlot>(Crosshair_R->Slot);
	Crosshair_T_Slot = Cast<UCanvasPanelSlot>(Crosshair_T->Slot);
	Crosshair_B_Slot = Cast<UCanvasPanelSlot>(Crosshair_B->Slot);

	if (ABaseController* BaseController = Cast<ABaseController>(GetOwningPlayer()))
	{
		BaseController->ChangeCrosshairSpread.AddUObject(this, &ThisClass::ChangeCrosshairSpread);
		BaseController->OnCrosshairHidden.AddUObject(this, &ThisClass::OnCrosshairHidden);
	}
}

void UCrosshair::ChangeCrosshairSpread(float Spread)
{
	if (Crosshair_L_Slot)
	{
		Crosshair_L_Slot->SetPosition(FVector2D(-Spread, 0.f));
	}
	if (Crosshair_R_Slot)
	{
		Crosshair_R_Slot->SetPosition(FVector2D(Spread, 0.f));
	}
	if (Crosshair_T_Slot)
	{
		Crosshair_T_Slot->SetPosition(FVector2D(0.f, -Spread));
	}
	if (Crosshair_B_Slot)
	{
		Crosshair_B_Slot->SetPosition(FVector2D(0.f, Spread));
	}
}

void UCrosshair::OnCrosshairHidden(bool bIsHidden)
{
	SetVisibility(bIsHidden ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}
