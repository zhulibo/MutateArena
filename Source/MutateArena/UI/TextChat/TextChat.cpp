#include "TextChat.h"

#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/UI/TextChat/MsgLineButton.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBoxSlot.h"
#include "MutateArena/System/UISubsystem.h"

#define LOCTEXT_NAMESPACE "UTextChat"

void UTextChat::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	
	MsgEditableTextBox->OnTextCommitted.AddUniqueDynamic(this, &ThisClass::OnMsgCommitted);
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ShowTextChat.AddUObject(this, &ThisClass::ShowTextChat);
		UISubsystem->OnReceiveMsg.AddUObject(this, &ThisClass::ShowMsg);
	}
}

void UTextChat::ShowTextChat()
{
	MsgEditableTextBox->SetVisibility(ESlateVisibility::Visible);
	MsgContainer->SetScrollBarVisibility(ESlateVisibility::Visible);
	MsgEditableTextBox->SetFocus();
}

void UTextChat::OnMsgCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMsgCommitted"));

	if (CommitMethod == ETextCommit::OnEnter)
	{
		SendMsg();
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		// 游戏内失去焦点
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
		if (BaseController)
		{
			MsgEditableTextBox->SetVisibility(ESlateVisibility::Hidden);
			MsgContainer->SetScrollBarVisibility(ESlateVisibility::Hidden);
			BaseController->FocusGame();
		}
	}
}

void UTextChat::SendMsg()
{
	if (EOSSubsystem == nullptr) return;

	FString Msg = MsgEditableTextBox->GetText().ToString();

	if (Msg.Len() == 0)
	{
		NOTIFY(this, C_YELLOW, LOCTEXT("EmptyMsg", "Empty msg"));
		return;
	}
	if (Msg.Len() > 60)
	{
		NOTIFY(this, C_YELLOW, FText::Format(LOCTEXT("WordsLimit", "Words limit {0}/60"), FText::AsNumber(Msg.Len())));
		return;
	}

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	// 游戏内聊天
	if (BaseController)
	{
		if (BasePlayerState == nullptr) BasePlayerState = Cast<ABasePlayerState>(GetOwningPlayerState());
		if (BasePlayerState)
		{
			BaseController->ServerSendMsg(
				EMsgType::Msg,
				BasePlayerState->Team,
				BasePlayerState->GetPlayerName(),
				Msg
			);

			MsgEditableTextBox->SetText(FText());
		}
	}
	// 菜单大厅聊天
	else
	{
		// 两次信息一样时不能触发OnLobbyAttrChanged，所以加一个空格
		if (Msg == LastMsg)
		{
			Msg += TEXT(" ");
		}
		LastMsg = Msg;

		EOSSubsystem->ModifyLobbyMemberAttr(TMap<UE::Online::FSchemaAttributeId, UE::Online::FSchemaVariant>{
			{ LOBBY_MEMBER_MSG, Msg}
		});

		MsgEditableTextBox->SetText(FText());
	}
}

void UTextChat::ShowMsg(const EMsgType MsgType, const ETeam Team, const FString& PlayerName, const FString& Msg)
{
	if (GetWorld()->bIsTearingDown) return;

	UMsgLineButton* MsgLineButton = CreateWidget<UMsgLineButton>(this, MsgLineButtonClass);
	if (MsgLineButton == nullptr) return;

	if (UScrollBoxSlot* NewSlot = Cast<UScrollBoxSlot>(MsgContainer->AddChild(MsgLineButton)))
	{
		NewSlot->SetPadding(FMargin(5, 5, 5, 5));
	}

	ETeam LocalTeam = ETeam::NoTeam;
	// PlayerName
	MsgLineButton->PlayerName->SetText(FText::FromString(ULibraryCommon::ObfuscateName(PlayerName, this)));
	if (EOSSubsystem)
	{
		LocalTeam = EOSSubsystem->GetMemberTeam(EOSSubsystem->GetLocalMember());
		if (LocalTeam != ETeam::NoTeam && Team != ETeam::NoTeam)
		{
			if (LocalTeam == Team)
			{
				MsgLineButton->PlayerName->SetColorAndOpacity(C_BLUE);
				MsgLineButton->MsgTime->SetColorAndOpacity(C_BLUE);
			}
			else
			{
				MsgLineButton->PlayerName->SetColorAndOpacity(C_RED);
				MsgLineButton->MsgTime->SetColorAndOpacity(C_RED);
			}
		}
		else
		{
			MsgLineButton->PlayerName->SetColorAndOpacity(C_WHITE);
			MsgLineButton->MsgTime->SetColorAndOpacity(C_WHITE);
		}
	}

	// MsgTime
	MsgLineButton->MsgTime->SetText(FText::FromString(ULibraryCommon::GetNowFormatTime()));

	// Msg
	switch (MsgType)
	{
	case EMsgType::Msg:
		MsgLineButton->Msg->SetText(FText::FromString(ULibraryCommon::ObfuscateTextChat(Msg, this)));
		MsgLineButton->Msg->SetColorAndOpacity(C_WHITE);
		break;
	case EMsgType::Radio:
		MsgLineButton->Msg->SetText(FText::FromString(Msg));
		MsgLineButton->Msg->SetColorAndOpacity(LocalTeam == Team ? C_BLUE :C_RED);
		break;
	case EMsgType::Join:
		MsgLineButton->Msg->SetText(LOCTEXT("Joined", "Joined"));
		MsgLineButton->Msg->SetColorAndOpacity(C_GREY);
		break;
	case EMsgType::Left:
		MsgLineButton->Msg->SetText(LOCTEXT("Left", "Left"));
		MsgLineButton->Msg->SetColorAndOpacity(C_GREY);
		break;
	case EMsgType::Kicked:
		MsgLineButton->Msg->SetText(LOCTEXT("Kicked", "Kicked"));
		MsgLineButton->Msg->SetColorAndOpacity(C_GREY);
		break;
	case EMsgType::Disconnected:
		MsgLineButton->Msg->SetText(LOCTEXT("Disconnected", "Disconnected"));
		MsgLineButton->Msg->SetColorAndOpacity(C_GREY);
		break;
	case EMsgType::Closed:
		MsgLineButton->Msg->SetText(LOCTEXT("Closed", "Closed"));
		MsgLineButton->Msg->SetColorAndOpacity(C_GREY);
		break;
	case EMsgType::Start:
		MsgLineButton->Msg->SetText(LOCTEXT("GameStart", "Game starts"));
		MsgLineButton->Msg->SetColorAndOpacity(C_YELLOW);
		break;
	case EMsgType::HostChange:
		MsgLineButton->Msg->SetText(LOCTEXT("BecomeHost", "Becomes the host"));
		MsgLineButton->Msg->SetColorAndOpacity(C_YELLOW);
		break;
	case EMsgType::ServerNameChange:
		MsgLineButton->Msg->SetText(FText::Format(LOCTEXT("ServerNameChange", "Server name change to {0}"), FText::FromString(Msg)));
		MsgLineButton->Msg->SetColorAndOpacity(C_YELLOW);
		break;
	case EMsgType::ModeNameChange:
		MsgLineButton->Msg->SetText(FText::Format(LOCTEXT("ModeChange", "Mode change to {0}"), FText::FromString(Msg)));
		MsgLineButton->Msg->SetColorAndOpacity(C_YELLOW);
		break;
	case EMsgType::MapNameChange:
		MsgLineButton->Msg->SetText(FText::Format(LOCTEXT("MapChange", "Map change to {0}"), FText::FromString(Msg)));
		MsgLineButton->Msg->SetColorAndOpacity(C_YELLOW);
		break;
	case EMsgType::MatchRoundChange:
		MsgLineButton->Msg->SetText(FText::Format(LOCTEXT("MatchRoundChange", "Match round change to {0}"), FText::FromString(Msg)));
		MsgLineButton->Msg->SetColorAndOpacity(C_YELLOW);
		break;
	case EMsgType::MatchTimeChange:
		MsgLineButton->Msg->SetText(FText::Format(LOCTEXT("MatchTimeChange", "Match time change to {0}"), FText::FromString(Msg)));
		MsgLineButton->Msg->SetColorAndOpacity(C_YELLOW);
		break;
	}

	if (MsgContainer->GetChildrenCount() > 50)
	{
		MsgContainer->RemoveChildAt(0);
	}

	// UE_LOG(LogTemp, Warning, TEXT("MsgContainerHoverButton->IsHovered() %d"), MsgContainerHoverButton->IsHovered());
	// UE_LOG(LogTemp, Warning, TEXT("MsgContainer->HasFocusedDescendants() %d"), MsgContainer->HasFocusedDescendants());

	// 未查看历史聊天时，滑到底部
	if (!MsgContainerHoverButton->IsHovered() && !MsgContainer->HasFocusedDescendants())
	{
		// ScrollToEnd多行文字有bug
		MsgContainer->SetScrollOffset(MsgContainer->GetDesiredSize().Y); // 应为滚动高度 - 窗口高度
	}
}

#undef LOCTEXT_NAMESPACE
