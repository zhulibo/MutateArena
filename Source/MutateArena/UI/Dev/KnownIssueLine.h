#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "KnownIssueLine.generated.h"

UCLASS()
class MUTATEARENA_API UKnownIssueLine : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* IssueText;
	
};
