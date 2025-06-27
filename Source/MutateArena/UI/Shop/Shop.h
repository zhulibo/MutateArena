#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "DataRegistryId.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Shop.generated.h"

struct FEquipmentMain;
struct FHumanCharacterMain;

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

	TMap<FDataRegistryId, const uint8*> HumanCharacterMains;
	TMap<FDataRegistryId, const uint8*> EquipmentMains;

	UPROPERTY(meta = (BindWidget))
	class UWrapBox* ItemButtonContainer;

	void OnQueryOffersComplete(bool bWasSuccessful);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UItemButton> EquipmentButtonClass;
	void AddEquipmentButton(const FOffer& Offer, const FEquipmentMain& EquipmentMain);

	UPROPERTY(EditAnywhere)
	TSubclassOf<UItemButton> CharacterButtonClass;
	void AddCharacterButton(const FOffer& Offer, const FHumanCharacterMain& HumanCharacterMain);

	void OnItemButtonClicked(UItemButton* ItemButton);
	void OnCheckoutComplete(bool bWasSuccessful, TOptional<FString> String);

};
