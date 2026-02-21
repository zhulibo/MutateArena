#include "CommonHUD.h"

#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "DamageLogLine.h"
#include "MutateArena/UI/HUD/CommonHUD/KillLogLine.h"
#include "Spectator.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/UI/GameLayout.h"
#include "MutateArena/UI/TextChat/TextChat.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "MutateArena/System/UISubsystem.h"

#define LOCTEXT_NAMESPACE "UCommonHUD"

void UCommonHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ChangeAnnouncement.AddUObject(this, &ThisClass::OnAnnouncementChange);
		UISubsystem->OnKillStreakChange.AddUObject(this, &ThisClass::OnKillStreakChange);
		UISubsystem->OnHUDStateChange.AddUObject(this, &ThisClass::OnHUDStateChange);
		UISubsystem->OnCauseDamage.AddUObject(this, &ThisClass::OnCauseDamage);
		UISubsystem->OnAddKillLog.AddUObject(this, &ThisClass::OnAddKillLog);
		UISubsystem->OnAFKHosting.AddUObject(this, &ThisClass::OnAFKHosting);
	}
	
	// 默认隐藏聊天输入框
	TextChat->MsgEditableTextBox->SetVisibility(ESlateVisibility::Hidden);
	TextChat->MsgContainer->SetScrollBarVisibility(ESlateVisibility::Hidden);
}

void UCommonHUD::OnAnnouncementChange(FText Text)
{
	Announcement->SetText(Text);
}

void UCommonHUD::OnKillStreakChange(int Num)
{
	if (Num > 1)
	{
		KillStreak->SetText(FText::FromString(FString::Printf(TEXT("%d KILL"), Num)));
	}
	else
	{
		KillStreak->SetText(FText());
	}
}

void UCommonHUD::OnAddKillLog(ABasePlayerState* AttackerState, const FText& CauserName, ABasePlayerState* DamagedState)
{
	if (AttackerState == nullptr || DamagedState == nullptr || GetWorld()->bIsTearingDown) return;

	if (KillLogContainer && KillLogLineClass)
	{
		if (UKillLogLine* KillLogLine = CreateWidget<UKillLogLine>(this, KillLogLineClass))
		{
			if (LocalPlayerState == nullptr) LocalPlayerState = GetOwningPlayerState<ABasePlayerState>();
			KillLogContainer->AddChild(KillLogLine);

			// 攻击者
			if (AttackerState)
			{
				
				KillLogLine->AttackerPlayer->SetText(FText::FromString(ULibraryCommon::ObfuscateName(AttackerState->GetPlayerName(), this)));
				if (LocalPlayerState)
				{
					if (LocalPlayerState->Team == AttackerState->Team)
					{
						KillLogLine->AttackerPlayer->SetColorAndOpacity(C_BLUE);
					}
					else
					{
						KillLogLine->AttackerPlayer->SetColorAndOpacity(C_RED);
					}
				}
			}

			// 击杀原因
			KillLogLine->CauserName->SetText(CauserName);

			// 死亡者
			if (DamagedState)
			{
				KillLogLine->DamagedPlayer->SetText(FText::FromString(ULibraryCommon::ObfuscateName(DamagedState->GetPlayerName(), this)));
				if (LocalPlayerState)
				{
					if (LocalPlayerState->Team == DamagedState->Team)
					{
						KillLogLine->DamagedPlayer->SetColorAndOpacity(C_BLUE);
					}
					else
					{
						KillLogLine->DamagedPlayer->SetColorAndOpacity(C_RED);
					}
				}
			}

			KillLogLine->PlayAnimation(KillLogLine->AppearanceAnim);
			
			// 限制击杀日志条数
			if (KillLogContainer->GetChildrenCount() > 5)
			{
				KillLogContainer->RemoveChildAt(0);
			}
		}
	}
}

void UCommonHUD::OnHUDStateChange(EHUDState HUDState)
{
	if (HUDState == EHUDState::Spectating)
	{
		Spectator->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Spectator->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// DamageLogContainer被旋转了180度，文字又倒转180度
void UCommonHUD::OnCauseDamage(float Num)
{
	if (DamageLogContainer == nullptr || DamageLogLineClass == nullptr) return;

	UDamageLogLine* DamageLogLine = nullptr;
	int32 MaxDamageLogCount = 5;

	// 1. 优先从容器内部寻找闲置（被隐藏）的 Widget 复用
	for (int32 i = 0; i < DamageLogContainer->GetChildrenCount(); ++i)
	{
		UDamageLogLine* Child = Cast<UDamageLogLine>(DamageLogContainer->GetChildAt(i));
		if (Child && Child->GetVisibility() == ESlateVisibility::Collapsed)
		{
			DamageLogLine = Child;
			// 移出再加入，它会被放到数组末尾
			DamageLogContainer->RemoveChild(Child);
			DamageLogContainer->AddChild(Child);
			break;
		}
	}

	// 如果没有找到闲置的 Widget
	if (!DamageLogLine)
	{
		if (DamageLogContainer->GetChildrenCount() >= MaxDamageLogCount)
		{
			// 如果已经达到数量上限，强制复用最老的
			DamageLogLine = Cast<UDamageLogLine>(DamageLogContainer->GetChildAt(0));
			if (DamageLogLine)
			{
				DamageLogContainer->RemoveChild(DamageLogLine);
				DamageLogContainer->AddChild(DamageLogLine);
			}
		}
		else
		{
			// 还没达到上限，直接创建新的
			DamageLogLine = CreateWidget<UDamageLogLine>(this, DamageLogLineClass);
			if (DamageLogLine)
			{
				DamageLogContainer->AddChild(DamageLogLine);
			}
		}
	}

	if (DamageLogLine)
	{
		DamageLogLine->SetVisibility(ESlateVisibility::Visible);

		static FNumberFormattingOptions Opts;
		static bool bOptsInitialized = false;
		if (!bOptsInitialized)
		{
			Opts.RoundingMode = ERoundingMode::ToNegativeInfinity;
			Opts.SetUseGrouping(false);
			Opts.SetMaximumFractionalDigits(0);
			bOptsInitialized = true;
		}

		FText NumericText = FText::AsNumber(Num, &Opts);
		DamageLogLine->Damage->SetText(FText::Format(FText::FromString(TEXT("+{0}")), NumericText)); 
		DamageLogLine->Damage->SetColorAndOpacity(C_YELLOW);
		
		DamageLogLine->PlayAnimation(DamageLogLine->AppearanceAnim);
	}

	GetWorld()->GetTimerManager().SetTimer(DamageLogTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (!DamageLogContainer) return;

		for (int32 i = 0; i < DamageLogContainer->GetChildrenCount(); ++i)
		{
			if (UWidget* Child = DamageLogContainer->GetChildAt(i))
			{
				Child->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}), 5.f, false);
}

void UCommonHUD::OnAFKHosting(bool bIsHosting)
{
	HostingBox->SetVisibility(bIsHosting ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
