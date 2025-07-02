#include "Shop.h"
#include "CommonTextBlock.h"
#include "DataRegistrySubsystem.h"
#include "ItemButton.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/Data/CharacterType.h"
#include "Components/WrapBox.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Components/WrapBoxSlot.h"

#define LOCTEXT_NAMESPACE "UShop"

void UShop::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 获取角色和装备数据
	if (UDataRegistry* DataRegistry = UDataRegistrySubsystem::Get()->GetRegistryForType(DR_HUMAN_CHARACTER_MAIN))
	{
		const UScriptStruct* OutStruct;
		DataRegistry->GetAllCachedItems(HumanCharacterMains, OutStruct);
	}
	if (UDataRegistry* DataRegistry = UDataRegistrySubsystem::Get()->GetRegistryForType(DR_EQUIPMENT_MAIN))
	{
		const UScriptStruct* OutStruct;
		DataRegistry->GetAllCachedItems(EquipmentMains, OutStruct);
	}

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnQueryOffersComplete.AddUObject(this, &ThisClass::OnQueryOffersComplete);
		EOSSubsystem->OnCheckoutComplete.AddUObject(this, &ThisClass::OnCheckoutComplete);
	}
}

void UShop::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	if (EOSSubsystem)
	{
		EOSSubsystem->QueryOffers();
	}
}

UWidget* UShop::NativeGetDesiredFocusTarget() const
{
	return ItemButtonContainer->GetChildAt(0);
}

// 查询商品列表完成
void UShop::OnQueryOffersComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (EOSSubsystem)
		{
			ItemButtonContainer->ClearChildren();
			
			// 获取商品列表
			TArray<FOffer> Offers = EOSSubsystem->GetOffers();

			for (int32 i = 0; i < Offers.Num(); ++i)
			{
				// 判断是装备还是角色
				bool bIsEquipment = false;

				for (const TPair<FDataRegistryId, const uint8*>& Pair : EquipmentMains)
				{
					FEquipmentMain EquipmentMain = *reinterpret_cast<const FEquipmentMain*>(Pair.Value);
					if (Offers[i].OfferId == EquipmentMain.OfferId)
					{
						AddEquipmentButton(Offers[i], EquipmentMain);
						bIsEquipment = true;
						break;
					}
				}

				if (bIsEquipment) continue;

				for (const TPair<FDataRegistryId, const uint8*>& Pair : HumanCharacterMains)
				{
					FHumanCharacterMain HumanCharacterMain = *reinterpret_cast<const FHumanCharacterMain*>(Pair.Value);
					if (Offers[i].OfferId == HumanCharacterMain.OfferId)
					{
						AddCharacterButton(Offers[i], HumanCharacterMain);
						break;
					}
				}
			}

			NativeGetDesiredFocusTarget()->SetFocus();
		}
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("QueryOffers", "Get items failed"));
	}
}

// 添加装备按钮
void UShop::AddEquipmentButton(const FOffer& Offer, const FEquipmentMain& EquipmentMain)
{
	if (UItemButton* EquipmentButton = CreateWidget<UItemButton>(this, EquipmentButtonClass))
	{
		EquipmentButton->Offer = Offer;
		FText ButtonText = FText();
		FText::FindTextInLiveTable_Advanced(CULTURE_EQUIPMENT, EquipmentMain.ShowName, ButtonText);
		EquipmentButton->ItemName->SetText(ButtonText);
		EquipmentButton->Price->SetText(Offer.FormattedPrice);
		EquipmentButton->OnClicked().AddUObject(this, &ThisClass::OnItemButtonClicked, EquipmentButton);
		if (UWrapBoxSlot* NewSlot = Cast<UWrapBoxSlot>(ItemButtonContainer->AddChild(EquipmentButton)))
		{
			NewSlot->SetPadding(FMargin(0, 0, 20, 20));
		}
	}
}

// 添加角色按钮
void UShop::AddCharacterButton(const FOffer& Offer, const FHumanCharacterMain& HumanCharacterMain)
{
	if (UItemButton* CharacterButton = CreateWidget<UItemButton>(this, CharacterButtonClass))
	{
		CharacterButton->Offer = Offer;
		FText ButtonText = FText();
		FText::FindTextInLiveTable_Advanced(CULTURE_HUMAN, HumanCharacterMain.ShowName, ButtonText);
		CharacterButton->ItemName->SetText(ButtonText);
		CharacterButton->Price->SetText(Offer.FormattedPrice);
		CharacterButton->OnClicked().AddUObject(this, &ThisClass::OnItemButtonClicked, CharacterButton);
		if (UWrapBoxSlot* NewSlot = Cast<UWrapBoxSlot>(ItemButtonContainer->AddChild(CharacterButton)))
		{
			NewSlot->SetPadding(FMargin(0, 0, 20, 20));
		}
	}
}

// 点击购买商品
void UShop::OnItemButtonClicked(UItemButton* ItemButton)
{
	if (EOSSubsystem)
	{
		TArray<FPurchaseOffer> Offers;
		Offers.Add(FPurchaseOffer{ ItemButton->Offer.OfferId, 1 });
		EOSSubsystem->Checkout(Offers);
	}
}

void UShop::OnCheckoutComplete(bool bWasSuccessful, TOptional<FString> String)
{
	if(bWasSuccessful)
	{
		NOTIFY(this, C_GREEN, LOCTEXT("CheckoutSuccess", "Buy item success"));
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("CheckoutFailed", "Buy item failed"));
	}
}

#undef LOCTEXT_NAMESPACE
