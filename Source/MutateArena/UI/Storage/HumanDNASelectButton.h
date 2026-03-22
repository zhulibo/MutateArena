#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "HumanDNASelectButton.generated.h"

class UHumanDNAAsset;
enum class EHumanDNA : uint8;

UCLASS(Abstract)
class MUTATEARENA_API UHumanDNASelectButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	void Setup(UHumanDNAAsset* DNAData);
	
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* DisplayName;
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Desc;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* Icon;
	
	UPROPERTY()
	EHumanDNA DNAType;

};
