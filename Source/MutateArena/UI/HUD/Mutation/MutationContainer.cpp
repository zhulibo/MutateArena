#include "MutationContainer.h"

#include "MutationHuman.h"
#include "MutationMutant.h"
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/UI/GameLayout.h"
#include "Components/DynamicEntryBox.h"
#include "MutateArena/System/UISubsystem.h"

void UMutationContainer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// HACK 提前实例化UMutationHuman/UMutationMutant，避免委托触发时还未绑定
	MutationHuman = Cast<UMutationHuman>(EntryBox_MutationHuman->CreateEntry());
	MutationMutant = Cast<UMutationMutant>(EntryBox_MutationMutant->CreateEntry());
	EntryBox_MutationHuman->Reset();
	EntryBox_MutationMutant->Reset();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnTeamChange.AddUObject(this, &ThisClass::OnTeamChange);
		UISubsystem->OnHUDStateChange.AddUObject(this, &ThisClass::OnHUDStateChange);
	}
}

void UMutationContainer::OnTeamChange(ETeam Team)
{
	if (Team == ETeam::Team1)
	{
		EntryBox_MutationHuman->Reset();
		EntryBox_MutationMutant->Reset();
		MutationHuman = Cast<UMutationHuman>(EntryBox_MutationHuman->CreateEntry());
	}
	else if (Team == ETeam::Team2)
	{
		EntryBox_MutationHuman->Reset();
		EntryBox_MutationMutant->Reset();
		MutationMutant = Cast<UMutationMutant>(EntryBox_MutationMutant->CreateEntry());
	}
}

void UMutationContainer::OnHUDStateChange(EHUDState HUDState)
{
	if (HUDState == EHUDState::Spectating)
	{
		EntryBox_MutationMutant->Reset();
	}
}
