#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "ScoreBoardLineButton.generated.h"

UCLASS()
class MUTATEARENA_API UScoreBoardLineButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Player;
	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* DNA1;
	UPROPERTY(meta = (BindWidget))
	UCommonLazyImage* DNA2;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Damage;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Death;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Survive;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Infect;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Platform;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Input;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Ping;

};
