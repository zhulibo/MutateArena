#include "MutationContainer.h"
#include "MutationHuman.h"
#include "MutationMutant.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/UI/GameLayout.h"

void UMutationContainer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnLocalTeamChange.AddUObject(this, &ThisClass::OnTeamChange);
		UISubsystem->OnHUDStateChange.AddUObject(this, &ThisClass::OnHUDStateChange);
	}
}

void UMutationContainer::NativeDestruct()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnLocalTeamChange.RemoveAll(this);
		UISubsystem->OnHUDStateChange.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UMutationContainer::OnTeamChange(ETeam Team)
{
	if (Team == ETeam::Team1)
	{
		MutationHuman->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		MutationMutant->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (Team == ETeam::Team2)
	{
		MutationHuman->SetVisibility(ESlateVisibility::Collapsed);
		MutationMutant->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UMutationContainer::OnHUDStateChange(EHUDState HUDState)
{
	if (HUDState == EHUDState::Spectating)
	{
		MutationHuman->SetVisibility(ESlateVisibility::Collapsed);
		MutationMutant->SetVisibility(ESlateVisibility::Collapsed);
	}
}
