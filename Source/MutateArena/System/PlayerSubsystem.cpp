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
			EOSSubsystem->OnLoginComplete.AddUObject(this, &ThisClass::OnLoginComplete);
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

void UPlayerSubsystem::ShowNotify(const FColor DisplayColor, const FText& Msg)
{
	if (NotifyLayout)
	{
		NotifyLayout->AddNotify(DisplayColor, Msg);
	}
}

void UPlayerSubsystem::Login(ECoolLoginType LoginType, FString Id, FString Token)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (EOSSubsystem == nullptr) EOSSubsystem = LocalPlayer->GetGameInstance()->GetSubsystem<UEOSSubsystem>();
		if (EOSSubsystem)
		{
			EOSSubsystem->Login(LocalPlayer->GetPlatformUserId(), LoginType, Id, Token);
		}
	}
}

void UPlayerSubsystem::OnLoginComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		LevelTravel(MAP_MENU);
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("LoginFailed", "Login failed"));
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
	
		LevelTravel(MAP_LOGIN);
	}
	else if (AuthLoginStatusChanged.LoginStatus == ELoginStatus::UsingLocalProfile)
	{
		bShowNotify_UsingLocalProfile = true;
	
		LevelTravel(MAP_LOGIN);
	}
}

void UPlayerSubsystem::LevelTravel(FString Url)
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld()))
		{
			PlayerController->ClientTravel(Url, ETravelType::TRAVEL_Absolute);
		}
	}
}

void UPlayerSubsystem::ShowLoginNotify()
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
		if (EOSSubsystem->CurrentLobby)
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

	// IsDead状态只需持续很短时间
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::SetIsAlive, 0.02f);
}

void UPlayerSubsystem::SetIsAlive()
{
	IsDead = false;
}

#undef LOCTEXT_NAMESPACE
