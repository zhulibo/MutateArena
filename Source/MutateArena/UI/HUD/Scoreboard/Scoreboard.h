#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Scoreboard.generated.h"

UCLASS()
class MUTATEARENA_API UScoreboard : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class ABaseGameState* BaseGameState;
	
	UPROPERTY()
	class UEOSSubsystem* EOSSubsystem;

	void ShowScoreboard(bool bIsShow);

	FTimerHandle TimerHandle;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UScoreBoardLineButton> ScoreBoardLineButtonClass;
	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* ScoreBoardContainer;
	UFUNCTION()
	void RefreshScoreBoard();

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Server;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* FPS;

};
