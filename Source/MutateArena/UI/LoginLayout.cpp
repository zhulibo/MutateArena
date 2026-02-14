#include "LoginLayout.h"

#include "CommonTextBlock.h"
#include "MutateArena/PlayerControllers/LoginController.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/System/PlayerSubsystem.h"
#include "Common/CommonButton.h"
#include "Components/SizeBox.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Utils/LibraryNotify.h"

#define LOCTEXT_NAMESPACE "ULoginLayout"

void ULoginLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FString Token;
	FParse::Value(FCommandLine::Get(), TEXT("AUTH_PASSWORD="), Token);
	LoginButton->OnClicked().AddUObject(this, &ThisClass::OnLoginButtonClicked, ECoolLoginType::ExchangeCode, FString(), Token);

#if UE_BUILD_DEVELOPMENT
	ShowDevContainer();
#endif

	DevButton->OnClicked().AddUObject(this, &ThisClass::OnDevButtonClicked);
	
	QuitButton->OnClicked().AddUObject(this, &ThisClass::OnQuitButtonClicked);

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnLoginComplete.AddUObject(this, &ThisClass::OnLoginComplete);
	}
}

UWidget* ULoginLayout::NativeGetDesiredFocusTarget() const
{
	return LoginButton;
}

void ULoginLayout::OnLoginButtonClicked(ECoolLoginType LoginType, FString Id, FString Token)
{
	if (LoginController == nullptr) LoginController = Cast<ALoginController>(GetOwningPlayer());
	if (LoginController)
	{
		if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
		if (EOSSubsystem)
		{
			EOSSubsystem->Login(LoginController->GetPlatformUserId(), LoginType, Id, Token);
			
			LoginStatus->SetText(LOCTEXT("Logging", "Logging..."));
			LoginButton->SetIsEnabled(false);
		}
	}
}

void ULoginLayout::OnLoginComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		LoginStatus->SetText(LOCTEXT("Redirecting", "Redirecting..."));
		
		if (LoginController == nullptr) LoginController = Cast<ALoginController>(GetOwningPlayer());
		if (LoginController)
		{
			LoginController->ClientTravel(MAP_MENU, ETravelType::TRAVEL_Absolute);
		}
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("LoginFailed", "Login failed"));
		
		LoginButton->SetIsEnabled(true);
		LoginStatus->SetText(LOCTEXT("LoginFailed", "Login failed"));
	}
}

void ULoginLayout::ShowDevContainer()
{
	if (DevContainer->GetVisibility() == ESlateVisibility::Visible) return;
	
	DevContainer->SetVisibility(ESlateVisibility::Visible);

	Login1Button->ButtonText->SetText(FText::FromString(TEXT("dust9923")));
	Login1Button->OnClicked().AddUObject(this, &ThisClass::OnLoginButtonClicked, ECoolLoginType::DevAuth, FString(TEXT("127.0.0.1:2333")), FString(TEXT("dust9923")));

	Login2Button->ButtonText->SetText(FText::FromString(TEXT("MutateArena-g")));
	Login2Button->OnClicked().AddUObject(this, &ThisClass::OnLoginButtonClicked, ECoolLoginType::DevAuth, FString(TEXT("127.0.0.1:2333")), FString(TEXT("MutateArena-g")));

	Login3Button->ButtonText->SetText(FText::FromString(TEXT("MutateArena-qq")));
	Login3Button->OnClicked().AddUObject(this, &ThisClass::OnLoginButtonClicked, ECoolLoginType::DevAuth, FString(TEXT("127.0.0.1:2333")), FString(TEXT("MutateArena-qq")));
	
	Login4Button->ButtonText->SetText(FText::FromString(TEXT("MutateArena-163")));
	Login4Button->OnClicked().AddUObject(this, &ThisClass::OnLoginButtonClicked, ECoolLoginType::DevAuth, FString(TEXT("127.0.0.1:2333")), FString(TEXT("MutateArena-163")));
}

void ULoginLayout::OnDevButtonClicked()
{
	ShowDevContainer();
}

void ULoginLayout::OnQuitButtonClicked()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
