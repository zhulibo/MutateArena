#include "EnvironmentManager.h"
#include "EnvironmentAsset.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/System/GameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkyLightComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/AssetSubsystem.h"

AEnvironmentManager::AEnvironmentManager()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	TransitionTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TransitionTimeline"));
}

void AEnvironmentManager::BeginPlay()
{
	Super::BeginPlay();

	if (TransitionCurve)
	{
		FOnTimelineFloat TimelineProgress;
		TimelineProgress.BindUFunction(this, FName("OnTimelineTick"));
		TransitionTimeline->AddInterpFloat(TransitionCurve, TimelineProgress);
	}
}

void AEnvironmentManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnvironmentManager, ActiveEnvironments);
}

void AEnvironmentManager::Reset()
{
	Super::Reset();

	// 跨回合对局：清空上一局角色的旧 GE Handle 映射，保留环境状态
	if (HasAuthority())
	{
		ActiveEnvEffectsMap.Empty();
	}
}

bool AEnvironmentManager::ActivateEnvironment(FGameplayTag EnvironmentTag)
{
	// 1. 安全检查与权限判断
	if (!HasAuthority() || !EnvironmentTag.IsValid())
	{
		return false;
	}

	// 2. 获取资产并验证
	UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (!AssetSubsystem)
	{
		return false;
	}

	UEnvironmentAsset* NewEnvironment = AssetSubsystem->GetEnvironmentAssetByTag(EnvironmentTag);
	if (!NewEnvironment)
	{
		return false;
	}

	// 3. 查重：如果当前队列中已经存在同 Tag 的环境，直接忽略
	for (const FActiveEnvironment& Env : ActiveEnvironments)
	{
		if (Env.EnvAsset && Env.EnvAsset->EnvironmentTag == EnvironmentTag)
		{
			return false;
		}
	}

	FActiveEnvironment NewInstance(NewEnvironment);

	// 4. 给当前存活玩家应用效果
	if (ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>())
	{
		TArray<ABasePlayerState*> PlayerStates;
		BaseGameState->GetPlayerStates(TOptional<ETeam>(), PlayerStates);

		for (ABasePlayerState* PS : PlayerStates)
		{
			if (PS)
			{
				if (ABaseCharacter* BaseChar = Cast<ABaseCharacter>(PS->GetPawn()))
				{
					if (!BaseChar->bIsDead)
					{
						ApplyEnvironmentInstanceToCharacter(NewInstance, BaseChar);
					}
				}
			}
		}
	}

	// 5. 加入同步队列
	ActiveEnvironments.Add(NewInstance);

	// 6. 手动调用一次视觉更新
	OnRep_ActiveEnvironments();

	return true;
}

// 玩家重生或中途加入(JIP)时调用
void AEnvironmentManager::ApplyAllActiveEnvironmentsToCharacter(ABaseCharacter* TargetCharacter)
{
	if (!HasAuthority() || !TargetCharacter) return;

	for (const FActiveEnvironment& Env : ActiveEnvironments)
	{
		ApplyEnvironmentInstanceToCharacter(Env, TargetCharacter);
	}
}

void AEnvironmentManager::ClearEnvironmentEffectsForASC(UAbilitySystemComponent* ASC)
{
	if (!ASC) return;

	for (auto& EnvMap : ActiveEnvEffectsMap)
	{
		if (TArray<FActiveGameplayEffectHandle>* Handles = EnvMap.Value.Find(ASC))
		{
			for (const FActiveGameplayEffectHandle& Handle : *Handles)
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
			EnvMap.Value.Remove(ASC);
		}
	}
}

void AEnvironmentManager::ApplyEnvironmentInstanceToCharacter(const FActiveEnvironment& EnvInstance, ABaseCharacter* TargetCharacter)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetCharacter);
	if (!ASC || !EnvInstance.EnvAsset) return;

	const TArray<TSubclassOf<UGameplayEffect>>* EffectsToApply = nullptr;

	// 使用 Actor 的 FName 标签判定阵营
	if (TargetCharacter->ActorHasTag(Tags::CharacterHuman))
	{
		EffectsToApply = &EnvInstance.EnvAsset->HumanEffects;
	}
	else if (TargetCharacter->ActorHasTag(Tags::CharacterMutant))
	{
		EffectsToApply = &EnvInstance.EnvAsset->MutantEffects;
	}

	if (EffectsToApply)
	{
		TArray<FActiveGameplayEffectHandle> Handles;
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(this, this);

		for (auto& GEClass : *EffectsToApply)
		{
			if (GEClass)
			{
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GEClass, 1.0f, Context);
				if (SpecHandle.IsValid())
				{
					FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					if (ActiveHandle.WasSuccessfullyApplied())
					{
						Handles.Add(ActiveHandle);
					}
				}
			}
		}

		if (Handles.Num() > 0)
		{
			// 使用 EnvironmentTag 作为底层映射的主键
			ActiveEnvEffectsMap.FindOrAdd(EnvInstance.EnvAsset->EnvironmentTag).FindOrAdd(ASC).Append(Handles);
		}
	}
}

void AEnvironmentManager::DeactivateEnvironment(FGameplayTag EnvironmentTag)
{
	if (!HasAuthority() || !EnvironmentTag.IsValid()) return;

	for (int32 i = 0; i < ActiveEnvironments.Num(); ++i)
	{
		if (ActiveEnvironments[i].EnvAsset && ActiveEnvironments[i].EnvAsset->EnvironmentTag == EnvironmentTag)
		{
			// 清理并移除该标签环境产生的所有 GE
			RemoveEnvironmentEffects(EnvironmentTag);
			ActiveEnvironments.RemoveAt(i);

			// 触发视觉恢复
			OnRep_ActiveEnvironments();
			break;
		}
	}
}

bool AEnvironmentManager::IsEnvironmentActive(FGameplayTag EnvironmentTag)
{
	if (!EnvironmentTag.IsValid())
	{
		return false;
	}

	for (const FActiveEnvironment& Env : ActiveEnvironments)
	{
		if (Env.EnvAsset && Env.EnvAsset->EnvironmentTag == EnvironmentTag)
		{
			return true;
		}
	}
	return false;
}

void AEnvironmentManager::RemoveEnvironmentEffects(FGameplayTag EnvironmentTag)
{
	if (auto* ASCMap = ActiveEnvEffectsMap.Find(EnvironmentTag))
	{
		for (auto& Pair : *ASCMap)
		{
			if (UAbilitySystemComponent* ASC = Pair.Key.Get())
			{
				for (const FActiveGameplayEffectHandle& Handle : Pair.Value)
				{
					ASC->RemoveActiveGameplayEffect(Handle);
				}
			}
		}
		ActiveEnvEffectsMap.Remove(EnvironmentTag);
	}
}

void AEnvironmentManager::OnRep_ActiveEnvironments()
{
	RecalculateVisualTargets();
}

void AEnvironmentManager::RecalculateVisualTargets()
{
	// 1. 扫描所有生效的环境，确保它们的参数已经记录在初始缓存字典中
	for (const FActiveEnvironment& Env : ActiveEnvironments)
	{
		if (!Env.EnvAsset) continue;

		for (const auto& ScalarPair : Env.EnvAsset->ScalarParameters)
		{
			// 如果是第一次遇到这个参数，去物理组件上读取其原始值并缓存
			if (!InitialScalarParams.Contains(ScalarPair.Key))
			{
				float BaseValue = 0.f;
				if (ScalarPair.Key.MatchesTag(TAG_ENV_UDS))
				{
					BaseValue = ReceiveReadUDSScalarParam(ScalarPair.Key);
				}
				else if (ScalarPair.Key.MatchesTag(TAG_ENV_UDW))
				{
					BaseValue = ReceiveReadUDWScalarParam(ScalarPair.Key);
				}
				else if (MPC_Env)
				{
					BaseValue = UKismetMaterialLibrary::GetScalarParameterValue(GetWorld(), MPC_Env, ScalarPair.Key.GetTagName());
				}

				InitialScalarParams.Add(ScalarPair.Key, BaseValue);

				// 把初始值赋值给当前值，不然 StartScalarParams 第一次找不到键
				if (!CurrentScalarParams.Contains(ScalarPair.Key))
				{
					CurrentScalarParams.Add(ScalarPair.Key, BaseValue);
				}
			}
		}

		for (const auto& ColorPair : Env.EnvAsset->ColorParameters)
		{
			if (!InitialColorParams.Contains(ColorPair.Key))
			{
				FLinearColor BaseColor = FLinearColor::White;
				if (ColorPair.Key.MatchesTag(TAG_ENV_UDS))
				{
					BaseColor = ReceiveReadUDSColorParam(ColorPair.Key);
				}
				else if (ColorPair.Key.MatchesTag(TAG_ENV_UDW))
				{
					BaseColor = ReceiveReadUDWColorParam(ColorPair.Key);
				}
				else if (MPC_Env)
				{
					BaseColor = UKismetMaterialLibrary::GetVectorParameterValue(GetWorld(), MPC_Env, ColorPair.Key.GetTagName());
				}

				InitialColorParams.Add(ColorPair.Key, BaseColor);

				if (!CurrentColorParams.Contains(ColorPair.Key))
				{
					CurrentColorParams.Add(ColorPair.Key, BaseColor);
				}
			}
		}
	}

	// 2. 先用默认值，然后覆盖环境值，这样当环境被Deactivate，且没有其他环境接管该参数时，能降级到初始值。
	TargetScalarParams = InitialScalarParams;
	TargetColorParams = InitialColorParams;

	// 3. 覆盖环境值
	for (const FActiveEnvironment& Env : ActiveEnvironments)
	{
		if (!Env.EnvAsset) continue;

		for (const auto& ScalarPair : Env.EnvAsset->ScalarParameters)
		{
			TargetScalarParams[ScalarPair.Key] = ScalarPair.Value;
		}
		for (const auto& ColorPair : Env.EnvAsset->ColorParameters)
		{
			TargetColorParams[ColorPair.Key] = ColorPair.Value;
		}
	}

	// 4. 将当前状态存为过渡起点
	StartScalarParams = CurrentScalarParams;
	StartColorParams = CurrentColorParams;

	// 5. 播放时间轴执行平滑过渡
	if (TransitionTimeline)
	{
		TransitionTimeline->PlayFromStart();
	}
}

void AEnvironmentManager::OnTimelineTick(float Value)
{
	// 处理浮点参数插值
	for (const auto& TargetPair : TargetScalarParams)
	{
		float StartVal = StartScalarParams.Contains(TargetPair.Key) ? StartScalarParams[TargetPair.Key] : 0.0f;
		float LerpedVal = FMath::Lerp(StartVal, TargetPair.Value, Value);

		// Add 方法的行为是添加或更新
		CurrentScalarParams.Add(TargetPair.Key, LerpedVal);

		if (TargetPair.Key.MatchesTag(TAG_ENV_UDS))
		{
			UE_LOG(LogTemp, Warning, TEXT("ReceiveUpdateUDSScalarParam TargetPair.Key, LerpedVal: %s, %f"), *TargetPair.Key.ToString(), LerpedVal);
			ReceiveUpdateUDSScalarParam(TargetPair.Key, LerpedVal);
		}
		else if (TargetPair.Key.MatchesTag(TAG_ENV_UDW))
		{
			UE_LOG(LogTemp, Warning, TEXT("ReceiveUpdateUDWScalarParam TargetPair.Key, LerpedVal: %s, %f"), *TargetPair.Key.ToString(), LerpedVal);
			ReceiveUpdateUDWScalarParam(TargetPair.Key, LerpedVal);
		}
		else if (MPC_Env)
		{
			UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), MPC_Env, TargetPair.Key.GetTagName(), LerpedVal);
		}
	}

	// 处理颜色参数插值
	for (const auto& TargetPair : TargetColorParams)
	{
		FLinearColor StartVal = StartColorParams.Contains(TargetPair.Key) ? StartColorParams[TargetPair.Key] : FLinearColor::White;
		FLinearColor LerpedVal = FMath::Lerp(StartVal, TargetPair.Value, Value);

		CurrentColorParams.Add(TargetPair.Key, LerpedVal);

		if (TargetPair.Key.MatchesTag(TAG_ENV_UDS))
		{
			UE_LOG(LogTemp, Warning, TEXT("ReceiveUpdateUDSColorParam TargetPair.Key, LerpedVal: %s, %s"), *TargetPair.Key.ToString(), *LerpedVal.ToString());
			ReceiveUpdateUDSColorParam(TargetPair.Key, LerpedVal);
		}
		else if (TargetPair.Key.MatchesTag(TAG_ENV_UDW))
		{
			UE_LOG(LogTemp, Warning, TEXT("ReceiveUpdateUDWColorParam TargetPair.Key, LerpedVal: %s, %s"), *TargetPair.Key.ToString(), *LerpedVal.ToString());
			ReceiveUpdateUDWColorParam(TargetPair.Key, LerpedVal);
		}
		else if (MPC_Env)
		{
			UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), MPC_Env, TargetPair.Key.GetTagName(), LerpedVal);
		}
	}
}
