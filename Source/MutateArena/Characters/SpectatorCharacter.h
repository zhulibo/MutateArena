#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "SpectatorCharacter.generated.h"

enum class ECommonInputType : uint8;

UENUM(BlueprintType)
enum class EPerspectiveType : uint8
{
	Free,
	FirstPerson,
	// TODO 实现第三人称
	ThirdPerson,

	None
};

UCLASS()
class MUTATEARENA_API ASpectatorCharacter : public ASpectatorPawn
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	UPROPERTY()
	class ABaseGameState* BaseGameState;
	UPROPERTY()
	class ABaseController* BaseController;
	
	void ScoreboardButtonPressed(const struct FInputActionValue& Value);
	void ScoreboardButtonReleased(const FInputActionValue& Value);
	void PauseMenuButtonPressed(const FInputActionValue& Value);

	void TextChat(const FInputActionValue& Value);

	UPROPERTY()
	EPerspectiveType PerspectiveType;
	void SwitchPerspective(const FInputActionValue& Value);
	void ViewPrevPlayer(const FInputActionValue& Value);
	void ViewNextPlayer(const FInputActionValue& Value);

};
