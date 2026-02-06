#include "Spectator.h"

#include "CommonTextBlock.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/UI/Common/CommonButton.h"

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

void USpectator::OnViewTargetChange(AActor* ViewTarget)
{
	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ViewTarget))
	{
		if (ABasePlayerState* BasePlayerState = Cast<ABasePlayerState>(BaseCharacter->GetPlayerState()))
		{
			CurPlayer->SetText(FText::FromString(BasePlayerState->GetPlayerName()));
		}
	}
}

#undef LOCTEXT_NAMESPACE
