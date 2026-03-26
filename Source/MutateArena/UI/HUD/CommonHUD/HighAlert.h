#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "HighAlert.generated.h"

class UMaterialInstanceDynamic;
class UImage;

UCLASS()
class MUTATEARENA_API UHighAlert : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void OnTeamChange(ETeam Team);
	
	ETeam TargetTeam = ETeam::NoTeam;

	UPROPERTY(meta = (BindWidget))
	UImage* AlertImage;

	UPROPERTY()
	UMaterialInstanceDynamic* AlertMID;

	// 警觉检测半径 
	UPROPERTY(EditAnywhere)
	float AlertRadius = 800.0f;

	// 射线检测更新频率 (秒) - 默认0.1秒(10Hz)，避免每帧执行开销巨大的Trace
	UPROPERTY(EditAnywhere)
	float TraceUpdateInterval = 0.1f;

	// 材质参数插值速度 - 配合降低的检测频率，使UI透明度过渡依然平滑
	UPROPERTY(EditAnywhere)
	float InterpSpeed = 10.0f;

	// 计时器累计
	float TimeSinceLastTrace = 0.0f;

	// 目标强度，用于平滑插值
	float TargetLeftAlert = 0.0f;
	float TargetRightAlert = 0.0f;
	float TargetBottomAlert = 0.0f;
	
	// 当前实际强度
	float CurrentLeftAlert = 0.0f;
	float CurrentRightAlert = 0.0f;
	float CurrentBottomAlert = 0.0f;
	
};
