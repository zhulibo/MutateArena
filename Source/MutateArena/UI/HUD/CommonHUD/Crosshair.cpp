#include "Crosshair.h"

#include "CommonBorder.h"
#include "Components/CanvasPanelSlot.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/System/UISubsystem.h"

void UCrosshair::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Crosshair_L_Slot = Cast<UCanvasPanelSlot>(Crosshair_L->Slot);
	Crosshair_R_Slot = Cast<UCanvasPanelSlot>(Crosshair_R->Slot);
	Crosshair_T_Slot = Cast<UCanvasPanelSlot>(Crosshair_T->Slot);
	Crosshair_B_Slot = Cast<UCanvasPanelSlot>(Crosshair_B->Slot);
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ChangeCrosshairSpread.AddUObject(this, &ThisClass::ChangeCrosshairSpread);
		UISubsystem->OnCrosshairHidden.AddUObject(this, &ThisClass::OnCrosshairHidden);
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
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetDefault<UDevSetting>()->bIsAdjustEquipmentSocketTransform) return;
	}

	SetVisibility(bIsHidden ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}
