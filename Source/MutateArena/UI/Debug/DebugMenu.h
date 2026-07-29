#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "DebugMenu.generated.h"

struct FGameplayTag;
class UCommonButton;
enum class EMsgResult : uint8;

UCLASS()
class MUTATEARENA_API UDebugMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class ABaseController* BaseController;
	UPROPERTY()
	class UUISubsystem* UISubsystem;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

	UPROPERTY()
	bool bWantToBack = true;
	UFUNCTION()
	void OnDeactivatedInternal();

	// Environment
	UPROPERTY(meta = (BindWidget))
	UCommonButton* EnvButton_BloodMoon;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* EnvButton_AcidRain;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* EnvButton_Snow;
	UPROPERTY(meta = (BindWidget))
	UCommonButton* EnvButton_Fog;
	void OnEnvButtonClicked(FGameplayTag EnvironmentTag);

};
