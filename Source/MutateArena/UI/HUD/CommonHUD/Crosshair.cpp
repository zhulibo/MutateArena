#include "Crosshair.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MutateArena/System/DevSetting.h"
#include "MutateArena/System/UISubsystem.h"

void UCrosshair::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CrosshairImage)
	{
		CrosshairMID = CrosshairImage->GetDynamicMaterial();
	}
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ChangeCrosshairSpread.AddUObject(this, &ThisClass::ChangeCrosshairSpread);
		UISubsystem->OnCrosshairHidden.AddUObject(this, &ThisClass::OnCrosshairHidden);
	}
}

void UCrosshair::ChangeCrosshairSpread(float Spread)
{
	if (CrosshairMID)
	{
		CrosshairMID->SetScalarParameterValue(FName("Spread"), Spread);
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
