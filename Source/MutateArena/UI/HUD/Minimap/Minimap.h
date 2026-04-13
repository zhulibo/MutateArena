#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Minimap.generated.h"

class UImage;
class UCanvasPanel;
class UMinimapIcon;
class ABasePlayerState;

UCLASS()
class MUTATEARENA_API UMinimap : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UImage* MinimapImage;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override; 
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UFUNCTION()
	void UpdateTeammateIcons();

	UPROPERTY(EditAnywhere)
	TSubclassOf<UMinimapIcon> IconWidgetClass;

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* IconContainer;

	UPROPERTY()
	TMap<ABasePlayerState*, UMinimapIcon*> TeammateIconMap;
	
	UPROPERTY()
	UMinimapIcon* LocalPlayerIcon;
	
};