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

	// CooldownTag = FGameplayTag::RequestGameplayTag(TAG_Mutant_SKILL_CD);
}

void UMutationMutant::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	// TODO CD图标
	// if (MutantCharacter == nullptr) MutantCharacter = Cast<AMutantCharacter>(GetOwningPlayerPawn());
	// if (MutantCharacter && MutantCharacter->GetAbilitySystemComponent())
	// {
	// 	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTag.GetSingleTagContainer());
	// 	TArray<float> Times = MutantCharacter->GetAbilitySystemComponent()->GetActiveEffectsTimeRemaining(Query);
	// }
}

void UMutationMutant::OnMutantHealthChange(float TempHealth)
{
	FNumberFormattingOptions Opts;
	Opts.RoundingMode = ERoundingMode::ToPositiveInfinity; // 向上取整
	Opts.SetUseGrouping(false); // 不使用千位分隔符
	Opts.SetMaximumFractionalDigits(0);
	Health->SetText(FText::AsNumber(TempHealth, &Opts));
}

void UMutationMutant::OnSkillChange(bool bIsShow)
{
	SkillBox->SetVisibility(bIsShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

// TODO 想要CommonUI配合增强输入来触发这个按键，但是ABaseController::FocusGame后，SkillButton无法被点击，增强输入无法触发这个按键
// 技能逻辑通过增强输入的BindAction处理了，这个按钮现在只有显示作用，待把技能逻辑移到按钮点击上
void UMutationMutant::OnSkillButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnSkillButtonClicked"));
}

void UMutationMutant::OnRageChange(float TempRage)
{
	FNumberFormattingOptions Opts;
	Opts.RoundingMode = ERoundingMode::ToPositiveInfinity; // 向上取整
	Opts.SetUseGrouping(false); // 不使用千位分隔符
	Opts.SetMaximumFractionalDigits(0);
	Rage->SetText(FText::AsNumber(TempRage, &Opts));
}
