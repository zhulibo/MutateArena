#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Version.generated.h"

class UCommonTextBlock;

UCLASS()
class MUTATEARENA_API UVersion : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* ProjectVersion;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* EngineVersion;
	
};
