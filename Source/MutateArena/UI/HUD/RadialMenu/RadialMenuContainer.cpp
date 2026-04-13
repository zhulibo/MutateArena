#include "RadialMenuContainer.h"

#include "CommonTextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "RadialMenuEquipment.h"
#include "RadialMenuPaint.h"
#include "RadialMenuRadio.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Data/InputAsset.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/UISubsystem.h"

#define LOCTEXT_NAMESPACE "URadialMenuContainer"

void URadialMenuContainer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	MenuCategories.Add({TitleEquipment, RadialMenuEquipment});
	MenuCategories.Add({TitleRadio, RadialMenuRadio});
	MenuCategories.Add({TitlePaint, RadialMenuPaint});

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ShowRadialMenu.AddUObject(this, &ThisClass::ShowRadialMenu);
		UISubsystem->SwitchRadialMenu.AddUObject(this, &ThisClass::SwitchRadialMenu);
		UISubsystem->SelectRadialMenu.AddUObject(this, &ThisClass::SelectRadialMenu);
	}
}

void URadialMenuContainer::NativeDestruct()
{
	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ShowRadialMenu.RemoveAll(this);
		UISubsystem->SwitchRadialMenu.RemoveAll(this);
		UISubsystem->SelectRadialMenu.RemoveAll(this);
	}

	Super::NativeDestruct();
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

	// 动态注入径向菜单 IMC
	if (UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>())
	{
		if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->AddMappingContext(AssetSubsystem->InputAsset->RadialMenuMappingContext, 1000);
			}
		}
	}
	
	HumanCharacter = Cast<AHumanCharacter>(GetOwningPlayerPawn());
	MutantCharacter = Cast<AMutantCharacter>(GetOwningPlayerPawn());

	TitleEquipment->SetColorAndOpacity(C_YELLOW);
	TitleRadio->SetColorAndOpacity(C_WHITE);
	TitlePaint->SetColorAndOpacity(C_WHITE);

	RadialMenuEquipment->SetVisibility(ESlateVisibility::Visible);
	RadialMenuRadio->SetVisibility(ESlateVisibility::Hidden);
	RadialMenuPaint->SetVisibility(ESlateVisibility::Hidden);

	RadialMenuEquipment->RefreshRadialMenu();

	CurrentCategoryIndex = 0;

	FLinearColor FactionColor = FLinearColor(C_YELLOW);
	// if (HumanCharacter)
	// {
	// 	FactionColor = FLinearColor(C_RED);
	// }
	// else if (MutantCharacter)
	// {
	// 	FactionColor = FLinearColor(C_GREEN);
	// }

	for (FMenuCategory& Category : MenuCategories)
	{
		if (Category.RadialMenu)
		{
			Category.RadialMenu->ResetPointerInput();

			Category.RadialMenu->SetPointerColor(FactionColor);
			Category.RadialMenu->SetSegmentColor(FactionColor);
		}
	}

	SetVisibility(ESlateVisibility::Visible);
}

// 切换轮盘
void URadialMenuContainer::SwitchRadialMenu()
{
	if (GetVisibility() != ESlateVisibility::Visible || MenuCategories.IsEmpty()) return;

	// 隐藏并重置当前菜单
	MenuCategories[CurrentCategoryIndex].RadialMenu->SetVisibility(ESlateVisibility::Hidden);
	MenuCategories[CurrentCategoryIndex].TitleText->SetColorAndOpacity(C_WHITE);
	MenuCategories[CurrentCategoryIndex].RadialMenu->SetSelectedItem(-1);

	// 索引循环步进
	CurrentCategoryIndex = (CurrentCategoryIndex + 1) % MenuCategories.Num();

	// 显示并高亮新菜单
	MenuCategories[CurrentCategoryIndex].RadialMenu->SetVisibility(ESlateVisibility::Visible);
	MenuCategories[CurrentCategoryIndex].TitleText->SetColorAndOpacity(C_YELLOW);

	MenuCategories[CurrentCategoryIndex].RadialMenu->ResetPointerInput();
}

// 选择轮盘项
void URadialMenuContainer::SelectRadialMenu(double X, double Y)
{
	// UE_LOG(LogTemp, Warning, TEXT("SelectRadialMenu X %f Y %f"), X, Y); 
	// 防止轮盘未显示时触发
	if (GetVisibility() != ESlateVisibility::Visible) return;

	if (URadialMenuBase* RadialMenu = GetActiveRadialMenu())
	{
		// 直接将原始 X, Y 传递给视图层进行平滑处理
		RadialMenu->UpdatePointerInput(X, Y);
	}
}

// 关闭轮盘
void URadialMenuContainer::CloseRadialMenuInternal()
{
	if (CurrentCategoryIndex == 0)
	{
		if (RadialMenuEquipment->SelectedItemIndex != -1)
		{
			// 人类
			if (HumanCharacter)
			{
				if (RadialMenuEquipment->SelectedItemIndex == 0)
				{
					HumanCharacter->SendSwapEquipmentEvent(EEquipmentType::Primary);
				}
				else if (RadialMenuEquipment->SelectedItemIndex == 1)
				{
					HumanCharacter->SendSwapEquipmentEvent(EEquipmentType::Secondary);
				}
				else if (RadialMenuEquipment->SelectedItemIndex == 2)
				{
					HumanCharacter->SendSwapEquipmentEvent(EEquipmentType::Melee);
				}
				else if (RadialMenuEquipment->SelectedItemIndex == 3)
				{
					HumanCharacter->SendSwapEquipmentEvent(EEquipmentType::Throwing);
				}
			}
			// 突变体
			else if (MutantCharacter)
			{
			}
		}
	}
	else if (CurrentCategoryIndex == 1)
	{
		if (RadialMenuRadio->SelectedItemIndex != -1)
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
	}
	else if (CurrentCategoryIndex == 2)
	{
		if (RadialMenuPaint->SelectedItemIndex != -1)
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
	}

	// 动态移除径向菜单 IMC
	if (UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>())
	{
		if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->RemoveMappingContext(AssetSubsystem->InputAsset->RadialMenuMappingContext);
			}
		}
	}
	
	SetVisibility(ESlateVisibility::Hidden);
}

// 获取活跃的轮盘
URadialMenuBase* URadialMenuContainer::GetActiveRadialMenu()
{
	return MenuCategories.IsValidIndex(CurrentCategoryIndex) ? MenuCategories[CurrentCategoryIndex].RadialMenu : nullptr;
}

#undef LOCTEXT_NAMESPACE
