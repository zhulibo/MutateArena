#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputAsset.generated.h"

UCLASS()
class MUTATEARENA_API UInputAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Base
	UPROPERTY(EditAnywhere, Category = "Base")
	class UInputMappingContext* BaseMappingContext;

	UPROPERTY(EditAnywhere, Category = "Base")
	class UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* LookMouseAction;
	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* LookStickAction;
	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* CrouchAction;
	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* CrouchControllerAction;
	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* ScoreboardAction;
	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* PauseMenuAction;

	UPROPERTY(EditAnywhere, Category = "Base")
	UInputAction* TextChatAction;

	// Human
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputMappingContext* HumanMappingContext;

	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* AimAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* FireAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* ReloadAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* DropAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* SwapPrimaryEquipmentAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* SwapSecondaryEquipmentAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* SwapMeleeEquipmentAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* SwapThrowingEquipmentAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* SwapLastEquipmentAction;
	UPROPERTY(EditAnywhere, Category = "Human")
	UInputAction* SwapBetweenPrimarySecondaryEquipmentAction;

	// Mutant
	UPROPERTY(EditAnywhere, Category = "Mutant")
	UInputMappingContext* MutantMappingContext;

	UPROPERTY(EditAnywhere, Category = "Mutant")
	UInputAction* LightAttackAction;
	UPROPERTY(EditAnywhere, Category = "Mutant")
	UInputAction* HeavyAttackAction;
	UPROPERTY(EditAnywhere, Category = "Mutant")
	UInputAction* SkillAction;

	// Spectator
	UPROPERTY(EditAnywhere, Category = "Spectator")
	UInputMappingContext* SpectatorMappingContext;

	UPROPERTY(EditAnywhere, Category = "Spectator")
	UInputAction* SwitchPerspectiveAction;
	UPROPERTY(EditAnywhere, Category = "Spectator")
	UInputAction* ViewNextAction;
	UPROPERTY(EditAnywhere, Category = "Spectator")
	UInputAction* ViewPrevAction;
	
	// RadialMenu
	UPROPERTY(EditAnywhere, Category = "RadialMenu")
	UInputMappingContext* RadialMenuMappingContext;

	UPROPERTY(EditAnywhere, Category = "RadialMenu")
	UInputAction* RadialMenuAction;
	UPROPERTY(EditAnywhere, Category = "RadialMenu")
	UInputAction* RadialMenuSwitchAction;
	UPROPERTY(EditAnywhere, Category = "RadialMenu")
	UInputAction* RadialMenuSelectAction;

};
