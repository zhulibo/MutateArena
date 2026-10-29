#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Storage.generated.h"

#define STORAGE_TYPE_ALL FString(TEXT("All"))
#define STORAGE_TYPE_CHARACTER FString(TEXT("Character"))

class UDNAAsset2;
class UCharacterHumanAsset;
class UEquipmentAsset;
enum class EHumanCharacterName : uint8;
enum class EEquipmentName : uint8;
enum class EEquipmentSlot : uint8;

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
	void ApplyLoadoutData(class USaveGameLoadout* SaveGameLoadout);

	bool HasEquipment(EEquipmentName EquipmentName);
	bool HasHumanCharacter(EHumanCharacterName HumanCharacterName);

	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;

	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* StorageTypeButtonContainer;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UCommonButton> StorageTypeButtonClass;

	void CreateStorageTypeTabs();
	int32 TypeIndex = 0;
	void OnStorageTypeButtonClicked(class UCommonButton* CommonButton);

	void AddEquipments(FString EquipmentType);
	void AddHumanCharacters();

	UPROPERTY(meta = (BindWidget))
	class UWrapBox* StorageButtonContainer;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UStorageButton> EquipmentButtonClass;
	void AddEquipmentButton(UEquipmentAsset* EquipmentAsset);

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UStorageButton> CharacterButtonClass;
	void AddCharacterButton(UCharacterHumanAsset* CharacterAsset);

	void OnEquipmentButtonClicked(class UStorageButton* EquipmentButton);

	UPROPERTY(meta = (BindWidget))
	class UCommonActivatableWidgetSwitcher* LoadoutSwitcher;

	void SaveEquipmentsToLoadouts();

	void OnCharacterButtonClicked(class UStorageButton* CharacterButton);

	UPROPERTY(meta = (BindWidget))
	class UHuman* Human;

	UPROPERTY(meta = (BindWidget))
	class UDNAButton* DNAButton1;
	UPROPERTY(meta = (BindWidget))
	class UDNAButton* DNAButton2;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UDNASelectScreen> DNASelectScreenClass;

	void OnDNAButton1Clicked();
	void OnDNAButton2Clicked();
	void OpenDNASelectScreen(class UDNAButton* TargetButton);

	void OnDNASelected(class UDNAAsset2* SelectedDNA, class UDNAButton* TargetButton);
};
