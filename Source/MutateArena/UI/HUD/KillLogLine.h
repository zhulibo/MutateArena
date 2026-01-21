#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "KillLogLine.generated.h"

UCLASS()
class MUTATEARENA_API UKillLogLine : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* AttackerPlayer;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CauserName;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* DamagedPlayer;

};
