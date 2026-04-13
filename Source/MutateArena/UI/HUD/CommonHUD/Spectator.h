#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Spectator.generated.h"

UCLASS()
class MUTATEARENA_API USpectator : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonButton* SwitchPerspectiveButton;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* ViewPrevButton;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* ViewNextButton;

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* CurPlayer;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	
	UPROPERTY()
	class ABaseController* BaseController;

	void OnViewTargetChange(AActor* ViewTarget);

};
