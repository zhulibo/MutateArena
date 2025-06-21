#include "RadialMenuContainer.h"
#include "CommonTextBlock.h"
#include "RadialMenuBase.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/PlayerStates/TeamType.h"

#define LOCTEXT_NAMESPACE "URadialMenuContainer"

void URadialMenuContainer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetHumanRadialMenuText();

	if (ABaseController* BaseController = Cast<ABaseController>(GetOwningPlayer()))
	{
		BaseController->ShowRadialMenu.AddUObject(this, &ThisClass::ShowRadialMenu);
		BaseController->ChangeRadialMenu.AddUObject(this, &ThisClass::ChangeRadialMenu);
		BaseController->SelectRadialMenu.AddUObject(this, &ThisClass::SelectRadialMenu);
	}
	
	if (AMutationController* MutationController = Cast<AMutationController>(GetOwningPlayer()))
	{
		MutationController->OnTeamChange.AddUObject(this, &ThisClass::OnTeamChange);
	}
}

void URadialMenuContainer::ShowRadialMenu(bool bIsShow)
{
	if (bIsShow)
	{
		ShowRadialMenuInternal();
	}
	else
	{
		CloseRadialMenuInternal();
	}
}

// 显示轮盘
void URadialMenuContainer::ShowRadialMenuInternal()
{
	// Action设为了长按，且ETriggerEvent::Triggered，会频繁触发
	if (GetVisibility() == ESlateVisibility::Visible) return;

	HumanCharacter = Cast<AHumanCharacter>(GetOwningPlayerPawn());
	MutantCharacter = Cast<AMutantCharacter>(GetOwningPlayerPawn());

	TitleEquipment->SetColorAndOpacity(C_GREEN);
	TitleRadio->SetColorAndOpacity(C_WHITE);
	TitlePaint->SetColorAndOpacity(C_WHITE);

	RadialMenuEquipment->SetVisibility(ESlateVisibility::Visible);
	RadialMenuRadio->SetVisibility(ESlateVisibility::Hidden);
	RadialMenuPaint->SetVisibility(ESlateVisibility::Hidden);

	ActiveRadialMenuIndex = 1;

	SetSelectedItem();

	SetVisibility(ESlateVisibility::Visible);
}

// 设置轮盘选中项
void URadialMenuContainer::SetSelectedItem()
{
	if (HumanCharacter)
	{
		if (HumanCharacter->CombatComponent)
		{
			switch (HumanCharacter->CombatComponent->CurEquipmentType)
			{
			case EEquipmentType::Primary:
				RadialMenuEquipment->SetSelectedItem(1);
				break;
			case EEquipmentType::Secondary:
				RadialMenuEquipment->SetSelectedItem(2);
				break;
			case EEquipmentType::Melee:
				RadialMenuEquipment->SetSelectedItem(3);
				break;
			case EEquipmentType::Throwing:
				RadialMenuEquipment->SetSelectedItem(4);
				break;
			}
		}
	}
	else if (MutantCharacter)
	{

	}
}

// 切换轮盘
void URadialMenuContainer::ChangeRadialMenu()
{
	if (ActiveRadialMenuIndex == 1)
	{
		RadialMenuEquipment->SetVisibility(ESlateVisibility::Hidden);
		TitleEquipment->SetColorAndOpacity(C_WHITE);

		RadialMenuRadio->SetVisibility(ESlateVisibility::Visible);
		TitleRadio->SetColorAndOpacity(C_GREEN);

		ActiveRadialMenuIndex = 2;
	}
	else if (ActiveRadialMenuIndex == 2)
	{
		RadialMenuRadio->SetVisibility(ESlateVisibility::Hidden);
		TitleRadio->SetColorAndOpacity(C_WHITE);

		RadialMenuPaint->SetVisibility(ESlateVisibility::Visible);
		TitlePaint->SetColorAndOpacity(C_GREEN);

		ActiveRadialMenuIndex = 3;
	}
	else if (ActiveRadialMenuIndex == 3)
	{
		RadialMenuPaint->SetVisibility(ESlateVisibility::Hidden);
		TitlePaint->SetColorAndOpacity(C_WHITE);

		RadialMenuEquipment->SetVisibility(ESlateVisibility::Visible);
		TitleEquipment->SetColorAndOpacity(C_GREEN);

		ActiveRadialMenuIndex = 1;
	}
}

// 选择轮盘项
void URadialMenuContainer::SelectRadialMenu(double X, double Y)
{
	// UE_LOG(LogTemp, Warning, TEXT("Select %f, %f"), X, Y);
	if (X * X + Y * Y < .6) return;

	if (URadialMenuBase* RadialMenu = GetActiveRadialMenu())
	{
		float Angle = FMath::Atan2(Y, X) * 180 / PI;
		// UE_LOG(LogTemp, Warning, TEXT("Angle %f"), Angle);

		if (Angle > -112.5 && Angle <= -67.5)
		{
			RadialMenu->SetSelectedItem(0);
		}
		else if (Angle > -67.5 && Angle <= -22.5)
		{
			RadialMenu->SetSelectedItem(1);
		}
		else if (Angle > -22.5 && Angle <= 22.5)
		{
			RadialMenu->SetSelectedItem(2);
		}
		else if (Angle > 22.5 && Angle <= 67.5)
		{
			RadialMenu->SetSelectedItem(3);
		}
		else if (Angle > 67.5 && Angle <= 112.5)
		{
			RadialMenu->SetSelectedItem(4);
		}
		else if (Angle > 112.5 && Angle <= 157.5)
		{
			RadialMenu->SetSelectedItem(5);
		}
		else if (Angle > 157.5 || Angle <= -157.5)
		{
			RadialMenu->SetSelectedItem(6);
		}
		else if (Angle > -157.5 && Angle <= -112.5)
		{
			RadialMenu->SetSelectedItem(7);
		}
	}
}

// 关闭轮盘
void URadialMenuContainer::CloseRadialMenuInternal()
{
	if (ActiveRadialMenuIndex == 1)
	{
		if (RadialMenuEquipment->LastSelectedItemIndex != RadialMenuEquipment->SelectedItemIndex)
		{
			// 人类
			if (HumanCharacter)
			{
				if (RadialMenuEquipment->SelectedItemIndex == 0)
				{
					HumanCharacter->SwapPrimaryEquipmentButtonPressed();
				}
				else if (RadialMenuEquipment->SelectedItemIndex == 1)
				{
					HumanCharacter->SwapSecondaryEquipmentButtonPressed();
				}
				else if (RadialMenuEquipment->SelectedItemIndex == 2)
				{
					HumanCharacter->SwapMeleeEquipmentButtonPressed();
				}
				else if (RadialMenuEquipment->SelectedItemIndex == 3)
				{
					HumanCharacter->SwapThrowingEquipmentButtonPressed();
				}
			}
			// 突变体
			else if (MutantCharacter)
			{

			}
		}
	}
	else if (ActiveRadialMenuIndex == 2)
	{
		// 人类
		if (HumanCharacter)
		{
			HumanCharacter->SendRadio(RadialMenuRadio->SelectedItemIndex);
		}
		// 突变体
		else if (MutantCharacter)
		{
			MutantCharacter->SendRadio(RadialMenuRadio->SelectedItemIndex);
		}
	}
	else if (ActiveRadialMenuIndex == 3)
	{
		// 人类
		if (HumanCharacter)
		{
			HumanCharacter->SprayPaint(RadialMenuPaint->SelectedItemIndex);
		}
		// 突变体
		else if (MutantCharacter)
		{
			MutantCharacter->SprayPaint(RadialMenuPaint->SelectedItemIndex);
		}
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void URadialMenuContainer::OnTeamChange(ETeam Team)
{
	if (Team == ETeam::Team1)
	{
		SetHumanRadialMenuText();
	}
	else if (Team == ETeam::Team2)
	{
		SetMutantRadialMenuText();
	}
}

void URadialMenuContainer::SetHumanRadialMenuText()
{
	FText Item1Text = FText();
	FText Item2Text = FText();
	FText Item3Text = FText();
	FText Item4Text = FText();
	FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT_TYPE, TEXT("Primary"), Item1Text);
	FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT_TYPE, TEXT("Secondary"), Item2Text);
	FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT_TYPE, TEXT("Melee"), Item3Text);
	FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT_TYPE, TEXT("Throwing"), Item4Text);

	RadialMenuEquipment->Item1->SetText(Item1Text);
	RadialMenuEquipment->Item2->SetText(Item2Text);
	RadialMenuEquipment->Item3->SetText(Item3Text);
	RadialMenuEquipment->Item4->SetText(Item4Text);
	RadialMenuEquipment->Item5->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item6->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item7->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item8->SetText(FText::FromString(TEXT("-1")));
}

void URadialMenuContainer::SetMutantRadialMenuText()
{
	RadialMenuEquipment->Item1->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item2->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item3->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item4->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item5->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item6->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item7->SetText(FText::FromString(TEXT("-1")));
	RadialMenuEquipment->Item8->SetText(FText::FromString(TEXT("-1")));
}

// 获取活跃的轮盘
URadialMenuBase* URadialMenuContainer::GetActiveRadialMenu()
{
	switch (ActiveRadialMenuIndex)
	{
	case 1:
		return RadialMenuEquipment;
	case 2:
		return RadialMenuRadio;
	case 3:
		return RadialMenuPaint;
	default:
		return nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
