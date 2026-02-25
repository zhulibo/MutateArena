#include "MutationMutant.h"

#include "AbilitySystemComponent.h"
#include "CommonTextBlock.h"
#include "GameplayEffect.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "Components/HorizontalBox.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"

void UMutationMutant::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnMutantHealthChange.AddUObject(this, &ThisClass::OnMutantHealthChange);
		UISubsystem->OnSkillChange.AddUObject(this, &ThisClass::OnSkillChange);
		UISubsystem->OnRageChange.AddUObject(this, &ThisClass::OnRageChange);
	}

	SkillButton->OnClicked().AddUObject(this, &ThisClass::OnSkillButtonClicked);
}

void UMutationMutant::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (bIsSkillReady) return;

	if (MutantCharacter == nullptr) MutantCharacter = Cast<AMutantCharacter>(GetOwningPlayerPawn());
	if (MutantCharacter && MutantCharacter->GetAbilitySystemComponent())
	{
		FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(TAG_MUTANT_SKILL_CD));
		TArray<float> Times = MutantCharacter->GetAbilitySystemComponent()->GetActiveEffectsTimeRemaining(Query);
		if (Times.Num() > 0)
		{
			float TimeRemaining = Times[0];
			if (TimeRemaining > 1.0f)
			{
				SkillCD->SetText(FText::AsNumber(FMath::CeilToInt(TimeRemaining)));
			}
			else
			{
				FNumberFormattingOptions Opts;
				Opts.SetMaximumFractionalDigits(1);
				Opts.SetMinimumFractionalDigits(1);
				SkillCD->SetText(FText::AsNumber(TimeRemaining, &Opts));
			}
		}
	}
}

void UMutationMutant::OnMutantHealthChange(float TempHealth)
{
	FNumberFormattingOptions Opts;
	Opts.RoundingMode = ERoundingMode::ToPositiveInfinity; // 向上取整
	Opts.SetUseGrouping(false); // 不使用千位分隔符
	Opts.SetMaximumFractionalDigits(0);
	Health->SetText(FText::AsNumber(TempHealth, &Opts));
}

void UMutationMutant::OnSkillChange(bool bIsReady)
{
	// 设置透明度
	SkillButton->SetRenderOpacity(bIsReady ? 1.f : .2f);
	
	bIsSkillReady = bIsReady;
	
	if (bIsReady)
	{
		SkillCD->SetText(FText::FromString(""));
	}
}

// TODO 想要CommonUI配合增强输入来触发这个按键，但是不知道为什么增强输入无法触发这个按键
// 技能逻辑通过增强输入的BindAction处理了，这个按钮现在只有显示作用，待把技能逻辑移到按钮点击上
void UMutationMutant::OnSkillButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("UMutationMutant::OnSkillButtonClicked"));
}

void UMutationMutant::OnRageChange(float TempRage)
{
	FNumberFormattingOptions Opts;
	Opts.RoundingMode = ERoundingMode::ToPositiveInfinity; // 向上取整
	Opts.SetUseGrouping(false); // 不使用千位分隔符
	Opts.SetMaximumFractionalDigits(0);
	Rage->SetText(FText::AsNumber(TempRage, &Opts));
}
