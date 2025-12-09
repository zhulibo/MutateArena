#include "LoginController.h"

#include "MutateArena/System/PlayerSubsystem.h"
#include "MutateArena/UI/LoginLayout.h"
#include "Blueprint/UserWidget.h"

void ALoginController::BeginPlay()
{
	Super::BeginPlay();
	
	if (UPlayerSubsystem* PlayerSubsystem = ULocalPlayer::GetSubsystem<UPlayerSubsystem>(GetLocalPlayer()))
	{
		PlayerSubsystem->AddNotifyLayout();
		PlayerSubsystem->CheckIfNeedShowLoginNotify();
	}

	AddLoginLayout();
}

void ALoginController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ALoginController::AddLoginLayout()
{
	if (IsLocalController() && LoginLayoutClass)
	{
		if (ULoginLayout* LoginLayout = CreateWidget<ULoginLayout>(this, LoginLayoutClass))
		{
			LoginLayout->AddToViewport();
			
			LoginLayout->ActivateWidget();
			
			FInputModeUIOnly InputModeData;
			SetInputMode(InputModeData);
			SetShowMouseCursor(true);
		}
	}
}
