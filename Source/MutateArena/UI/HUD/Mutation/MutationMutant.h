#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "MutationMutant.generated.h"

enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API UMutationMutant : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	
	UPROPERTY()
	class AMutantCharacter* MutantCharacter;

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Health;
	void OnMutantHealthChange(float TempHealth);
	
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* SkillBox;
	UPROPERTY(meta = (BindWidget))
	class UCommonButton* SkillButton;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* SkillCD;
	bool bIsSkillReady = false;
	void OnSkillChange(bool bIsShow);
	void OnSkillButtonClicked();

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Rage;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* RageLabel;
	void OnRageChange(float TempRage);
	
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Level;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* LevelLabel;
	void OnLevelChange(float TempLevel);

};
