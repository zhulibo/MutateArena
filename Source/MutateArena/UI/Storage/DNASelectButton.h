#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "DNASelectButton.generated.h"

class UDNAAsset2;
enum class EDNA : uint8;

UCLASS(Abstract)
class MUTATEARENA_API UDNASelectButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	void Setup(UDNAAsset2* DNAData);
	
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* DisplayName;
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Desc;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* Icon;
	
	UPROPERTY()
	EDNA DNA;

};
