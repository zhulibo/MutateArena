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

    // 玩家重生或中途加入时调用，自动补齐当前所有生效的环境效果
    UFUNCTION()
    void ApplyAllActiveEnvironmentsToCharacter(ABaseCharacter* TargetCharacter);

    // 回合重启时清理
    virtual void Reset() override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(ReplicatedUsing = OnRep_ActiveEnvironments)
    TArray<FActiveEnvironment> ActiveEnvironments;

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

    // UDS 引用（在蓝图中用于转换和修改）
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* UltraDynamicSky;
    
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
    
    // 抛给蓝图：设置 UDS 标量参数
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveUpdateUDSScalarParam(FGameplayTag ParamTag, float Value);
    // 抛给蓝图：设置 UDS 颜色参数
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveUpdateUDSColorParam(FGameplayTag ParamTag, FLinearColor Value);
    // 抛给蓝图：读取 UDS 标量初始值
    UFUNCTION(BlueprintImplementableEvent)
    float ReceiveReadUDSScalarParam(FGameplayTag ParamTag);
    // 抛给蓝图：读取 UDS 颜色初始值
    UFUNCTION(BlueprintImplementableEvent)
    FLinearColor ReceiveReadUDSColorParam(FGameplayTag ParamTag);
};
