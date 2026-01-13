#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "DataRegistryId.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Storage.generated.h"

#define STORAGE_TYPE_ALL FString(TEXT("All"))
#define STORAGE_TYPE_CHARACTER FString(TEXT("Character"))

struct FEquipmentMain;
struct FHumanCharacterMain;
enum class EHumanCharacterName : uint8;
enum class EEquipmentName : uint8;
enum class EEquipmentType : uint8;

UCLASS()
class MUTATEARENA_API UStorage : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;

	void OnQueryEntitlementsComplete(bool bWasSuccessful);
	void OnEnumerateUserFilesComplete(bool bWasSuccessful);
	void OnReadUserFileComplete(bool bWasSuccessful, const FUserFileContentsRef& FileContents, const FString& Filename);
	void InitPlayerConfig(class USaveGameLoadout* SaveGameLoadout);
	bool HasEquipment(EEquipmentName EquipmentName, EEquipmentType EquipmentType);
	bool HasHumanCharacter(EHumanCharacterName HumanCharacterName);

	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;

	TMap<FDataRegistryId, const uint8*> HumanCharacterMains;
	TMap<FDataRegistryId, const uint8*> EquipmentMains;

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* StorageTypeButtonContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UCommonButton> StorageTypeButtonClass;
	void AddStorageTypeButton();

	void OnStorageTypeButtonClicked(UCommonButton* CommonButton);
	void AddEquipments(FString EquipmentType);
	void AddHumanCharacters();

	UPROPERTY(meta = (BindWidget))
	class UWrapBox* StorageButtonContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UStorageButton> EquipmentButtonClass;
	void AddEquipmentButton(FEquipmentMain EquipmentMain);
	UPROPERTY(EditAnywhere)
	TSubclassOf<UStorageButton> CharacterButtonClass;
	void AddCharacterButton(FHumanCharacterMain HumanCharacterMain);

	void OnEquipmentButtonClicked(class UStorageButton* CommonButton);
	UPROPERTY(meta = (BindWidget))
	class UCommonActivatableWidgetSwitcher* LoadoutSwitcher;
	void SaveEquipmentsToLoadouts();

	void OnCharacterButtonClicked(UStorageButton* CommonButton);
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Character;

	int32 TypeIndex = 0;
	
};
