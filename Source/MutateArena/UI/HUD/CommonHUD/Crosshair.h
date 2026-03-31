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
	class UImage* CrosshairImage;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class UMaterialInstanceDynamic* CrosshairMID;
	
	void ChangeCrosshairSpread(float Spread);
	void OnCrosshairHidden(bool bIsHidden);

};
