#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "OverheadWidget.generated.h"

UCLASS()
class MUTATEARENA_API UOverheadWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	friend class ABaseCharacter;

	void InitOverheadWidget();

	void OnMaxHealthChange(float MaxHealth);
	void OnHealthChange(float OldHealth, float NewHealth);

	bool bIsAllowShow = true;
	void ShowOverheadWidget(bool bIsShow);
	
	void PlayFlashbangEffect(float Speed);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	UPROPERTY()
	TObjectPtr<class ABaseGameState> BaseGameState;

	// 拥有此OverheadWidget的角色
	UPROPERTY()
	ABaseCharacter* BaseCharacter;
	UPROPERTY()
	class ABasePlayerState* BasePlayerState;

	// 本地角色
	UPROPERTY()
	class ABaseController* LocalBaseController;
	UPROPERTY()
	ABasePlayerState* LocalBasePlayerState;
	UPROPERTY()
	ABaseCharacter* LocalBaseCharacter;

	void OnMatchEnd();
	UPROPERTY()
	bool bCanUseSetTimerForNextTick = true;

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* PlayerName;
	void SetPlayerName();
	FTimerHandle TraceTimerHandle;
	void TraceOverheadWidget();

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* HealthBar;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HealthDec;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HealthInc;

	UPROPERTY(meta = (BindWidget))
	UCommonLazyImage* HealthBarLine;
	int32 GetHealthBarLineNum();

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeIn;

};
