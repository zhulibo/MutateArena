#pragma once

#include "CoreMinimal.h"
#include "Online/Commerce.h"
#include "CommonButtonBase.h"
#include "ItemButton.generated.h"

using namespace UE::Online;

UCLASS()
class MUTATEARENA_API UItemButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* TranslatedShowName;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Price;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImg;

	FOffer Offer;

};
