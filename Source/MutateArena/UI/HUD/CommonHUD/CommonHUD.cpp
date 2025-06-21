#include "CommonHUD.h"

#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "MutateArena/UI/HUD/KillLogLine.h"
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

#define LOCTEXT_NAMESPACE "UCommonHUD"

void UCommonHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ABaseController* BaseController = Cast<ABaseController>(GetOwningPlayer()))
	{
		BaseController->ChangeAnnouncement.AddUObject(this, &ThisClass::OnAnnouncementChange);
		BaseController->OnKillStreakChange.AddUObject(this, &ThisClass::OnKillStreakChange);
		BaseController->OnHUDStateChange.AddUObject(this, &ThisClass::OnHUDStateChange);
	}

	if (ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>())
	{
		BaseGameState->OnAddKillLog.AddUObject(this, &ThisClass::OnAddKillLog);
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
				
				KillLogLine->AttackerPlayer->SetText(FText::FromString(ULibraryCommon::ObfuscatePlayerName(AttackerState->GetPlayerName(), this)));
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
				KillLogLine->DamagedPlayer->SetText(FText::FromString(ULibraryCommon::ObfuscatePlayerName(DamagedState->GetPlayerName(), this)));
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

			// 限制击杀日志条数
			if (KillLogContainer->GetChildrenCount() > 6)
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

#undef LOCTEXT_NAMESPACE
