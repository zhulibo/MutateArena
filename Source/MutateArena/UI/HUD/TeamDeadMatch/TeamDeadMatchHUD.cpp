#include "TeamDeadMatchHUD.h"

#include "CommonTextBlock.h"
#include "MutateArena/PlayerControllers/TeamDeadMatchController.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UTeamDeadMatchHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ATeamDeadMatchController* TeamDeadMatchController = Cast<ATeamDeadMatchController>(GetOwningPlayer()))
	{
		TeamDeadMatchController->OnMatchCountdownChange.AddUObject(this, &ThisClass::OnMatchCountdownChange);
		TeamDeadMatchController->OnHumanHealthChange.AddUObject(this, &ThisClass::OnHumanHealthChange);
		TeamDeadMatchController->OnAmmoChange.AddUObject(this, &ThisClass::OnAmmoChange);
		TeamDeadMatchController->OnCarriedAmmoChange.AddUObject(this, &ThisClass::OnCarriedAmmoChange);
		TeamDeadMatchController->OnTeamScoreChange.AddUObject(this, &ThisClass::OnTeamScoreChange);
	}
}

void UTeamDeadMatchHUD::OnMatchCountdownChange(int32 CountdownTime)
{
	FString CountdownText = ULibraryCommon::GetFormatTime(CountdownTime);
	MatchCountdown->SetText(FText::FromString(CountdownText));
}

void UTeamDeadMatchHUD::OnHumanHealthChange(float TempHealth)
{
	FNumberFormattingOptions Opts;
	Opts.RoundingMode = ERoundingMode::ToPositiveInfinity; // 向上取整
	Opts.SetUseGrouping(false); // 不使用千位分隔符
	Opts.SetMaximumFractionalDigits(0);
	Health->SetText(FText::AsNumber(TempHealth, &Opts));
}

void UTeamDeadMatchHUD::OnAmmoChange(int32 TempAmmo)
{
	Ammo->SetText(FText::AsNumber(TempAmmo));
}

void UTeamDeadMatchHUD::OnCarriedAmmoChange(int32 TempCarriedAmmo)
{
	CarriedAmmo->SetText(FText::AsNumber(TempCarriedAmmo));
}

void UTeamDeadMatchHUD::OnTeamScoreChange(float Score, ETeam Team)
{
	switch (Team)
	{
	case ETeam::Team1:
		Team1Score->SetText(FText::AsNumber(Score));
		break;
	case ETeam::Team2:
		Team2Score->SetText(FText::AsNumber(Score));
		break;
	}
}
