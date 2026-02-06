#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "DataRegistryId.h"
#include "MutantSelect.generated.h"

enum class EMutantCharacterName : uint8;

UCLASS()
class MUTATEARENA_API UMutantSelect : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class ABasePlayerState* BasePlayerState;
	UPROPERTY()
	class AMutationGameState* MutationGameState;
	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	UPROPERTY()
	class UAbilitySystemComponent* AbilitySystemComponent;

	TMap<FDataRegistryId, const uint8*> MutantCharacterMain;

	UPROPERTY(meta = (BindWidget))
	class UCommonHierarchicalScrollBox* MutantSelectButtonContainer;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMutantSelectButton> MutantSelectButtonClass;
	UFUNCTION()
	void OnMutantSelectButtonClicked(EMutantCharacterName MutantCharacterName);

	void OnRoundEnded();

	void CloseMenu(bool bClosePauseMenu);

};
