#include "PlayerLineButton.h"

#include "CommonTextBlock.h"
#include "Engine/GameInstance.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/Utils/LibraryNotify.h"

#define LOCTEXT_NAMESPACE "UPlayerLineButton"

void UPlayerLineButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	OnHovered().AddUObject(this, &ThisClass::OnPlayerLineButtonHovered);
	OnUnhovered().AddUObject(this, &ThisClass::OnPlayerLineButtonUnhovered);

	PromotePlayerButton->OnClicked().AddUObject(this, &ThisClass::OnPromotePlayerButtonClicked);
	PromotePlayerButton->SetVisibility(ESlateVisibility::Hidden);

	KickPlayerButton->OnClicked().AddUObject(this, &ThisClass::OnKickPlayerButtonClicked);
	KickPlayerButton->SetVisibility(ESlateVisibility::Hidden);
	KickPlayerButton->SetIsEnabled(false); // TODO OnLobbyLeft无效，暂时禁用

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnPromoteLobbyMemberComplete.AddUObject(this, &ThisClass::OnPromoteLobbyMemberComplete);
		EOSSubsystem->OnKickLobbyMemberComplete.AddUObject(this, &ThisClass::OnKickLobbyMemberComplete);
	}
}

void UPlayerLineButton::OnPlayerLineButtonHovered()
{
	if (EOSSubsystem && EOSSubsystem->IsLobbyHost() && !EOSSubsystem->IsLocalMember(Member))
	{
		PromotePlayerButton->SetVisibility(ESlateVisibility::Visible);
		KickPlayerButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPlayerLineButton::OnPlayerLineButtonUnhovered()
{
	PromotePlayerButton->SetVisibility(ESlateVisibility::Hidden);
	KickPlayerButton->SetVisibility(ESlateVisibility::Hidden);
}

void UPlayerLineButton::OnPromotePlayerButtonClicked()
{
	if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->PromoteLobbyMember(Member->AccountId);
	}
}

void UPlayerLineButton::OnPromoteLobbyMemberComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("ChangeHostFailed", "Change host failed"));
	}
}

void UPlayerLineButton::OnKickPlayerButtonClicked()
{
	if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->KickLobbyMember(Member->AccountId);
	}
}

void UPlayerLineButton::OnKickLobbyMemberComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("KickPlayerFailed", "Kick player failed"));
	}
}

#undef LOCTEXT_NAMESPACE
