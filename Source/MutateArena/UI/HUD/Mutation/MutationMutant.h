#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "MutationMutant.generated.h"

enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API UMutationMutant : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	class AMutantCharacter* MutantCharacter;
	UPROPERTY()
	FGameplayTag CooldownTag;

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Health;
	void OnMutantHealthChange(float TempHealth);
	
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* SkillBox;
	UPROPERTY(meta = (BindWidget))
	class UCommonButton* SkillButton;
	void OnSkillChange(bool bIsShow);
	void OnSkillButtonClicked();

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Rage;
	void OnRageChange(float TempRage);

};
