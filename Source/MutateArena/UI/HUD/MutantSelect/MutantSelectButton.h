#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "MutantSelectButton.generated.h"

enum class EMutantCharacterName : uint8;

UCLASS()
class MUTATEARENA_API UMutantSelectButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Name;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Attribute;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Skill;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Desc;
	UPROPERTY()
	EMutantCharacterName MutantCharacterName;

};
