#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Crosshair.generated.h"

UCLASS()
class MUTATEARENA_API UCrosshair : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonBorder* Crosshair_L;
	UPROPERTY(meta = (BindWidget))
	UCommonBorder* Crosshair_R;
	UPROPERTY(meta = (BindWidget))
	UCommonBorder* Crosshair_T;
	UPROPERTY(meta = (BindWidget))
	UCommonBorder* Crosshair_B;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class UCanvasPanelSlot* Crosshair_L_Slot;
	UPROPERTY()
	UCanvasPanelSlot* Crosshair_R_Slot;
	UPROPERTY()
	UCanvasPanelSlot* Crosshair_T_Slot;
	UPROPERTY()
	UCanvasPanelSlot* Crosshair_B_Slot;
	
	void ChangeCrosshairSpread(float Spread);
	void OnCrosshairHidden(bool bIsHidden);

};
