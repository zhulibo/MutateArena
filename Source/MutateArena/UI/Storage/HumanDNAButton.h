#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "HumanDNAButton.generated.h"

enum class EHumanDNA : uint8;

UCLASS()
class MUTATEARENA_API UHumanDNAButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImg;

	UPROPERTY()
	EHumanDNA HumanDNA;

};
