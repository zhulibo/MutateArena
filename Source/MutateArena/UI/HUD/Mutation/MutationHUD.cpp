#include "MutationHUD.h"

#include "CommonTextBlock.h"
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UMutationHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnMatchCountdownChange.AddUObject(this, &ThisClass::OnMatchCountdownChange);
		UISubsystem->OnTeamNumChange.AddUObject(this, &ThisClass::OnTeamNumChange);
		UISubsystem->OnCurRoundChange.AddUObject(this, &ThisClass::OnCurRoundChange);
		UISubsystem->OnTotalRoundChange.AddUObject(this, &ThisClass::OnTotalRoundChange);
	}
}

void UMutationHUD::OnMatchCountdownChange(int32 CountdownTime)
{
	FString CountdownText = ULibraryCommon::GetFormatTime(CountdownTime);
	MatchCountdown->SetText(FText::FromString(CountdownText));
}

void UMutationHUD::OnTeamNumChange(int32 TeamNum, ETeam Team)
{
	switch (Team)
	{
	case ETeam::Team1:
		HumanNumber->SetText(FText::AsNumber(TeamNum));
		break;
	case ETeam::Team2:
		MutantNumber->SetText(FText::AsNumber(TeamNum));
		break;
	}
}

void UMutationHUD::OnCurRoundChange(int32 Round)
{
	CurRound->SetText(FText::AsNumber(Round));
}

void UMutationHUD::OnTotalRoundChange(int32 Round)
{
	TotalRound->SetText(FText::AsNumber(Round));
}
