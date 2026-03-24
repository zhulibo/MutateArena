#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "DNAButton.generated.h"

class UDNAAsset2;
enum class EDNA : uint8;

UCLASS(Abstract)
class MUTATEARENA_API UDNAButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* DisplayName;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* Icon;
	
	UPROPERTY()
	EDNA DNA;

	void UpdateDNAInfo(class UDNAAsset2* DNAData);
	
};
