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

	void ShowOverheadWidget(bool bIsShow);
	void PlayFlashbangEffect(float Speed);

	void OnMaxHealthChange(float MaxHealth);
	void OnHealthChange(float OldHealth, float NewHealth);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	TWeakObjectPtr<class ABaseGameState> BaseGameState;
	TWeakObjectPtr<class ABaseCharacter> BaseCharacter;
	TWeakObjectPtr<class ABasePlayerState> BasePlayerState;
	
	TWeakObjectPtr<class ABaseController> LocalBaseController;
	TWeakObjectPtr<class ABasePlayerState> LocalBasePlayerState;
	TWeakObjectPtr<class ABaseCharacter> LocalBaseCharacter;
	
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* PlayerName;
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* CT;
	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* HealthBar;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HealthDec;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HealthInc;
	UPROPERTY(meta = (BindWidget))
	UCommonLazyImage* HealthBarLine;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeIn;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* HealthBarMID;
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* HealthBarLineMID;
	
	bool bNeedUpdate = true;
	void MakeDirty();
	float TraceDistance = 3000.f;
	void InitOverheadWidget();
	FTimerHandle TraceTimerHandle;
	void TraceOverheadWidget();
	int32 GetHealthBarLineNum();
	bool bIsAllowShow = true;
	
	// 记录上一次处于准星范围内的时间
	float LastAimTime = -999.f;
	// 血条延迟显示的时间
	UPROPERTY()
	float HealthBarLingerTime = 2.f;
	
};