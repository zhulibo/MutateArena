#include "MeleeHUD.h"

#include "CommonTextBlock.h"
#include "MutateArena/PlayerControllers/MeleeController.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UMeleeHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (AMeleeController* MeleeController = Cast<AMeleeController>(GetOwningPlayer()))
	{
		MeleeController->OnMatchCountdownChange.AddUObject(this, &ThisClass::OnMatchCountdownChange);
		MeleeController->OnHumanHealthChange.AddUObject(this, &ThisClass::OnHumanHealthChange);
		MeleeController->OnAmmoChange.AddUObject(this, &ThisClass::OnAmmoChange);
		MeleeController->OnCarriedAmmoChange.AddUObject(this, &ThisClass::OnCarriedAmmoChange);
		MeleeController->OnTeamScoreChange.AddUObject(this, &ThisClass::OnTeamScoreChange);
	}
}

void UMeleeHUD::OnMatchCountdownChange(int32 CountdownTime)
{
	FString CountdownText = ULibraryCommon::GetFormatTime(CountdownTime);
	MatchCountdown->SetText(FText::FromString(CountdownText));
}

void UMeleeHUD::OnHumanHealthChange(float TempHealth)
{
	FNumberFormattingOptions Opts;
	Opts.RoundingMode = ERoundingMode::ToPositiveInfinity; // 向上取整
	Opts.SetUseGrouping(false); // 不使用千位分隔符
	Opts.SetMaximumFractionalDigits(0);
	Health->SetText(FText::AsNumber(TempHealth, &Opts));
}

void UMeleeHUD::OnAmmoChange(int32 TempAmmo)
{
	Ammo->SetText(FText::AsNumber(TempAmmo));
}

void UMeleeHUD::OnCarriedAmmoChange(int32 TempCarriedAmmo)
{
	CarriedAmmo->SetText(FText::AsNumber(TempCarriedAmmo));
}

void UMeleeHUD::OnTeamScoreChange(float Score, ETeam Team)
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
