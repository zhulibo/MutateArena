#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Shop.generated.h"

class UCharacterHumanAsset;
class UEquipmentSkinAsset;

UCLASS()
class MUTATEARENA_API UShop : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;

	UPROPERTY(meta = (BindWidget))
	class UWrapBox* ItemButtonContainer;

	void OnQueryOffersComplete(bool bWasSuccessful);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UItemButton> EquipmentButtonClass;
	void AddEquipmentButton(const FOffer& Offer, const UEquipmentSkinAsset* EquipmentAsset);

	UPROPERTY(EditAnywhere)
	TSubclassOf<UItemButton> CharacterButtonClass;
	void AddCharacterButton(const FOffer& Offer, const UCharacterHumanAsset* CharacterAsset);

	void OnItemButtonClicked(UItemButton* ItemButton);

	void OnCheckoutComplete(bool bWasSuccessful, TOptional<FString> String);
};
