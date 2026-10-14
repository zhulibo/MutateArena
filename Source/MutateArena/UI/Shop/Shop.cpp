#include "Shop.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "ItemButton.h"
#include "MutateArena/MutateArena.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/GameInstance.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "MutateArena/System/AssetSubsystem.h" 
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

#define LOCTEXT_NAMESPACE "UShop"

void UShop::NativeOnInitialized()
{
    Super::NativeOnInitialized();

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
    if (ItemButtonContainer->GetChildrenCount() > 0)
    {
        return ItemButtonContainer->GetChildAt(0);
    }
    return nullptr;
}

// 查询商品列表完成
void UShop::OnQueryOffersComplete(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
       if (EOSSubsystem)
       {
          ItemButtonContainer->ClearChildren();
          
          TArray<FOffer> Offers = EOSSubsystem->GetOffers();
          UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();

          if (!AssetSubsystem) return;

          for (int32 i = 0; i < Offers.Num(); ++i)
          {
             bool bMatched = false;

             // 判断是装备还是角色
             for (const auto& Pair : AssetSubsystem->CachedHumanCharacterAssets)
             {
                if (Pair.Value && Pair.Value->OfferId == Offers[i].OfferId)
                {
                   AddCharacterButton(Offers[i], Pair.Value);
                   bMatched = true;
                   break;
                }
             }

             if (bMatched) continue;

             for (const auto& Pair : AssetSubsystem->CachedEquipmentSkinAssets)
             {
                if (Pair.Value && Pair.Value->OfferId == Offers[i].OfferId)
                {
                   AddEquipmentButton(Offers[i], Pair.Value);
                   bMatched = true;
                   break;
                }
             }
             
          }

          if (UWidget* FocusTarget = NativeGetDesiredFocusTarget())
          {
              FocusTarget->SetFocus();
          }
       }
    }
    else
    {
       NOTIFY(this, Colors::Red, LOCTEXT("QueryOffers", "Get items failed"));
    }
}

// 添加装备按钮
void UShop::AddEquipmentButton(const FOffer& Offer, const UEquipmentSkinAsset* EquipmentAsset)
{
    if (!EquipmentAsset) return;

    if (UItemButton* EquipmentButton = CreateWidget<UItemButton>(this, EquipmentButtonClass))
    {
       EquipmentButton->Offer = Offer;
       EquipmentButton->ShowName->SetText(EquipmentAsset->SkinName);
       EquipmentButton->Price->SetText(Offer.FormattedPrice);
       EquipmentButton->ShowImg->SetBrushFromLazyTexture(EquipmentAsset->SkinIcon);
       
       EquipmentButton->OnClicked().AddUObject(this, &ThisClass::OnItemButtonClicked, EquipmentButton);
       if (UWrapBoxSlot* NewSlot = Cast<UWrapBoxSlot>(ItemButtonContainer->AddChild(EquipmentButton)))
       {
          NewSlot->SetPadding(FMargin(0, 0, 20, 20));
       }
    }
}

// 添加角色按钮
void UShop::AddCharacterButton(const FOffer& Offer, const UCharacterHumanAsset* CharacterAsset)
{
    if (!CharacterAsset) return;

    if (UItemButton* CharacterButton = CreateWidget<UItemButton>(this, CharacterButtonClass))
    {
       CharacterButton->Offer = Offer;
       CharacterButton->ShowName->SetText(CharacterAsset->ShowName);
       CharacterButton->Price->SetText(Offer.FormattedPrice);
       CharacterButton->ShowImg->SetBrushFromLazyTexture(CharacterAsset->ShowImg);
       
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
    if (EOSSubsystem && ItemButton)
    {
       TArray<FPurchaseOffer> Offers;
       Offers.Add(FPurchaseOffer{ ItemButton->Offer.OfferId, 1 });
       // TODO 附加内容 Add-On Offer.PurchaseLimit 为 1，后端应该做了限制，此处并未做限制
       EOSSubsystem->Checkout(Offers);
    }
}

void UShop::OnCheckoutComplete(bool bWasSuccessful, TOptional<FString> String)
{
    if(bWasSuccessful)
    {
       NOTIFY(this, Colors::Green, LOCTEXT("CheckoutSuccess", "Buy item success"));
    }
    else
    {
       NOTIFY(this, Colors::Red, LOCTEXT("CheckoutFailed", "Buy item failed"));
    }
}

#undef LOCTEXT_NAMESPACE