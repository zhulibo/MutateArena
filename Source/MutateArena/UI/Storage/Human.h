#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "Human.generated.h"

UCLASS()
class MUTATEARENA_API UHuman : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* TranslatedShowName;
	// UPROPERTY()
	// FString ShowName;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImgFullBody;

};
