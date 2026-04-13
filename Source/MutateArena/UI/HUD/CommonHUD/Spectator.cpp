#include "Spectator.h"

#include "CommonTextBlock.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "MutateArena/Utils/LibraryCommon.h"

#define LOCTEXT_NAMESPACE "USpectator"

void USpectator::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BaseController = Cast<ABaseController>(GetOwningPlayer());
	if (BaseController)
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
		{
			UISubsystem->OnViewTargetChange.AddUObject(this, &ThisClass::OnViewTargetChange);
		}
	}
}

void USpectator::NativeDestruct()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnViewTargetChange.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void USpectator::OnViewTargetChange(AActor* ViewTarget)
{
	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ViewTarget))
	{
		if (ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(BaseCharacter->GetPlayerState()))
		{
			CurPlayer->SetText(FText::FromString(ULibraryCommon::ObfuscateName(BasePlayerState->GetPlayerName(), this)));
		}
	}
}

#undef LOCTEXT_NAMESPACE
