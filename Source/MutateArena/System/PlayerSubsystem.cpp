#include "PlayerSubsystem.h"

#include "AssetSubsystem.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/UI/Notify/NotifyLayout.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Data/CommonAsset.h"

#define LOCTEXT_NAMESPACE "UPlayerSubsystem"

UPlayerSubsystem::UPlayerSubsystem()
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		EOSSubsystem = LocalPlayer->GetGameInstance()->GetSubsystem<UEOSSubsystem>();
		if (EOSSubsystem)
		{
			EOSSubsystem->OnLoginStatusChanged.AddUObject(this, &ThisClass::OnLoginStatusChanged);
			EOSSubsystem->OnUILobbyJoinRequested.AddUObject(this, &ThisClass::OnUILobbyJoinRequested);
		}
	}
}

void UPlayerSubsystem::Tick(float DeltaTime)
{
}

void UPlayerSubsystem::AddNotifyLayout()
{
	UAssetSubsystem* AssetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->CommonAsset)
	{
		NotifyLayout = CreateWidget<UNotifyLayout>(GetWorld(), AssetSubsystem->CommonAsset->NotifyLayoutClass);
		if (NotifyLayout)
		{
			NotifyLayout->AddToViewport(999);
		}
	}
}

void UPlayerSubsystem::AddNotify(const FColor DisplayColor, const FText& Msg)
{
	if (NotifyLayout)
	{
		NotifyLayout->AddNotify(DisplayColor, Msg);
	}
}

void UPlayerSubsystem::OnLoginStatusChanged(const FAuthLoginStatusChanged& AuthLoginStatusChanged)
{
	if (AuthLoginStatusChanged.LoginStatus == ELoginStatus::LoggedIn)
	{
		bShowNotify_LoggedIn = true;
	}
	else if (AuthLoginStatusChanged.LoginStatus == ELoginStatus::LoggedInReducedFunctionality)
	{
		bShowNotify_LoggedInReducedFunctionality = true;
	}
	else if (AuthLoginStatusChanged.LoginStatus == ELoginStatus::NotLoggedIn)
	{
		bShowNotify_NotLoggedIn = true;
	}
	else if (AuthLoginStatusChanged.LoginStatus == ELoginStatus::UsingLocalProfile)
	{
		bShowNotify_UsingLocalProfile = true;
	}

	if (AuthLoginStatusChanged.LoginStatus == ELoginStatus::NotLoggedIn || AuthLoginStatusChanged.LoginStatus == ELoginStatus::UsingLocalProfile)
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld()))
			{
				PlayerController->ClientTravel(MAP_LOGIN, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UPlayerSubsystem::CheckIfNeedShowLoginNotify()
{
	if (bShowNotify_LoggedIn)
	{
		NOTIFY(this, C_GREEN, LOCTEXT("LoginStatusChanged_LoggedIn", "Login status changed: LoggedIn"));
		bShowNotify_LoggedIn = false;
	}
	if (bShowNotify_LoggedInReducedFunctionality)
	{
		NOTIFY(this, C_WHITE, LOCTEXT("LoginStatusChanged_LoggedInReducedFunctionality", "Login status changed: LoggedInReducedFunctionality"));
		bShowNotify_LoggedInReducedFunctionality = false;
	}
	if (bShowNotify_NotLoggedIn)
	{
		NOTIFY(this, C_YELLOW, LOCTEXT("LoginStatusChanged_NotLoggedIn", "Login status changed: NotLoggedIn"));
		bShowNotify_NotLoggedIn = false;
	}
	if (bShowNotify_UsingLocalProfile)
	{
		NOTIFY(this, C_YELLOW, LOCTEXT("LoginStatusChanged_UsingLocalProfile", "Login status changed: UsingLocalProfile"));
		bShowNotify_UsingLocalProfile = false;
	}
}

void UPlayerSubsystem::OnUILobbyJoinRequested(const FUILobbyJoinRequested& UILobbyJoinRequested)
{
	if (EOSSubsystem)
	{
		if (EOSSubsystem->CurLobby)
		{
			NOTIFY(this, C_YELLOW, LOCTEXT("LeaveToAcceptInvitation", "You must leave current lobby to join new one"));

			return;
		}

		if (UILobbyJoinRequested.Result.IsOk())
		{
			EOSSubsystem->JoinLobby(UILobbyJoinRequested.Result.GetOkValue());
		}
	}
}

void UPlayerSubsystem::SetIsDead()
{
	IsDead = true;

	// 0.02s后恢复IsDead为false
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]() {
		IsDead = false;
	});
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, 0.02f,  false);
}

#undef LOCTEXT_NAMESPACE
