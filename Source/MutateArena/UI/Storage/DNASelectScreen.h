#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "DNASelectScreen.generated.h"

class UWrapBox;
enum class EDNA : uint8;
class UDNASelectButton;

DECLARE_DELEGATE_OneParam(FDNASelectComplete, class UDNAAsset2*);

UCLASS(Abstract)
class MUTATEARENA_API UDNASelectScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void Setup(FDNASelectComplete InCallback);

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> DNAContainer;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UDNASelectButton> DNASelectButtonClass;

	void OnDNAButtonClicked(UDNAAsset2* SelectedDNA);

private:
	FDNASelectComplete Callback;

};
