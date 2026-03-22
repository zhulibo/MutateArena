#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "HumanDNASelectScreen.generated.h"

class UWrapBox;
enum class EHumanDNA : uint8;
class UHumanDNASelectButton;

DECLARE_DELEGATE_OneParam(FHumanDNASelectComplete, class UHumanDNAAsset*);

UCLASS(Abstract)
class MUTATEARENA_API UHumanDNASelectScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void Setup(FHumanDNASelectComplete InCallback);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> DNAContainer;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UHumanDNASelectButton> HumanDNASelectButtonClass;

	void OnDNAButtonClicked(UHumanDNAAsset* SelectedDNA);

private:
	FHumanDNASelectComplete Callback;

};
