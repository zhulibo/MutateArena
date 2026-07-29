#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Components/TimelineComponent.h"
#include "EnvironmentManager.generated.h"

class AExponentialHeightFog;
class ASkyLight;
class UEnvironmentAsset;
class UAbilitySystemComponent;
class ABaseCharacter;
class ADirectionalLight;
class UMaterialParameterCollection;

// 用于网络同步的活动环境结构体
USTRUCT(BlueprintType)
struct FActiveEnvironment
{
	GENERATED_BODY()

	UPROPERTY()
	UEnvironmentAsset* EnvAsset;

	FActiveEnvironment() : EnvAsset(nullptr)
	{
	}

	FActiveEnvironment(UEnvironmentAsset* InAsset) : EnvAsset(InAsset)
	{
	}
};

UCLASS(Blueprintable)
class MUTATEARENA_API AEnvironmentManager : public AInfo
{
	GENERATED_BODY()

public:
	AEnvironmentManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	bool ActivateEnvironment(FGameplayTag EnvironmentTag);
	UFUNCTION()
	void DeactivateEnvironment(FGameplayTag EnvironmentTag);
	UFUNCTION()
	bool IsEnvironmentActive(FGameplayTag EnvironmentTag);

	// 玩家重生或中途加入时调用，自动补齐当前所有生效的环境效果
	UFUNCTION()
	void ApplyAllActiveEnvironmentsToCharacter(ABaseCharacter* TargetCharacter);
	// 清理指定角色身上的所有环境GE
	void ClearEnvironmentEffectsForASC(UAbilitySystemComponent* ASC);
	
	// 回合重启时清理
	virtual void Reset() override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_ActiveEnvironments)
	TArray<FActiveEnvironment> ActiveEnvironments;

protected:
	UFUNCTION()
	void OnRep_ActiveEnvironments();

private:
	// 记录每个环境赋予的所有 GE Handle
	TMap<FGameplayTag, TMap<TWeakObjectPtr<UAbilitySystemComponent>, TArray<FActiveGameplayEffectHandle>>> ActiveEnvEffectsMap;

	void RemoveEnvironmentEffects(FGameplayTag EnvironmentTag);
	void ApplyEnvironmentInstanceToCharacter(const FActiveEnvironment& EnvInstance, ABaseCharacter* TargetCharacter);

protected:
	// 视觉过渡与时间轴
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* TransitionTimeline;
	UPROPERTY(EditAnywhere)
	UCurveFloat* TransitionCurve;
	UFUNCTION()
	void OnTimelineTick(float Value);

	UPROPERTY(EditAnywhere)
	UMaterialParameterCollection* MPC_Env;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* UltraDynamicSky;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* UltraDynamicWeather;

	// 初始状态（用于当所有环境移除时恢复值）
	TMap<FGameplayTag, float> InitialScalarParams;
	TMap<FGameplayTag, FLinearColor> InitialColorParams;

	// 插值状态
	TMap<FGameplayTag, float> StartScalarParams;
	TMap<FGameplayTag, float> TargetScalarParams;
	TMap<FGameplayTag, float> CurrentScalarParams;
	TMap<FGameplayTag, FLinearColor> StartColorParams;
	TMap<FGameplayTag, FLinearColor> TargetColorParams;
	TMap<FGameplayTag, FLinearColor> CurrentColorParams;

	void RecalculateVisualTargets();

	// 更新蓝图UDS参数
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveUpdateUDSScalarParam(FGameplayTag ParamTag, float Value);
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveUpdateUDSColorParam(FGameplayTag ParamTag, FLinearColor Value);
	UFUNCTION(BlueprintImplementableEvent)
	float ReceiveReadUDSScalarParam(FGameplayTag ParamTag);
	UFUNCTION(BlueprintImplementableEvent)
	FLinearColor ReceiveReadUDSColorParam(FGameplayTag ParamTag);

	// 更新蓝图UDW参数
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveUpdateUDWScalarParam(FGameplayTag ParamTag, float Value);
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveUpdateUDWColorParam(FGameplayTag ParamTag, FLinearColor Value);
	UFUNCTION(BlueprintImplementableEvent)
	float ReceiveReadUDWScalarParam(FGameplayTag ParamTag);
	UFUNCTION(BlueprintImplementableEvent)
	FLinearColor ReceiveReadUDWColorParam(FGameplayTag ParamTag);
};
